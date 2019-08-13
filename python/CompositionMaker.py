"""
This module is developed to automated the process of making composite images in parallel for MappingAfrica project. The whole process
is consisted of two steps: 1) make planet ARD images and 2) call AFMapTSComposite (c-based exe) for making composites
The module can be called by using one of three modes: 1) tile-based, 2)csv-based and 3) aoi based
Tile and csv-based mode are mainly for testing usage; aoi-based mode is for on-production
Author: Su Ye (sye@clarku.edu)
"""

import boto3
import pandas as pd
import gdal
import geopandas as gpd
import osr
from shapely.geometry import mapping
from math import ceil
from datetime import datetime
import os
import click
from scipy import ndimage
import logging
import time
import yaml
import subprocess
import multiprocessing
from pytz import timezone
from fixed_thread_pool_executor import FixedThreadPoolExecutor
from osgeo import gdal_array
import numpy as np
import shutil

# functional exception
class FuncException(Exception):
    pass

# (this function has been abandoned in the current version,  cause the searching efficiency over s3 is low)
def get_matching_s3_keys(bucket, prefix='', suffix=''):
    """
    Generate the keys in an S3 bucket.
    arg:
        bucket: Name of the S3 bucket.
        prefix: Only fetch keys that start with this prefix (optional).
        suffix: Only fetch keys that end with this suffix (optional).
    return:
        (string) key
    """
    s3 = boto3.client('s3')
    kwargs = {'Bucket': bucket}

    # If the prefix is a single string (not a tuple of strings), we can
    # do the filtering directly in the S3 API.
    if isinstance(prefix, str):
        kwargs['Prefix'] = prefix

    while True:

        # The S3 API response is a large blob of metadata.
        # 'Contents' contains information about the listed objects.
        resp = s3.list_objects_v2(**kwargs)
        for obj in resp['Contents']:
            key = obj['Key']
            if key.startswith(prefix) and key.endswith(suffix):
                return key

        # The S3 API is paginated, returning up to 1000 keys at a time.
        # Pass the continuation token into the next response, until we
        # reach the final page (when this field is missing).
        try:
            kwargs['ContinuationToken'] = resp['NextContinuationToken']
        except KeyError:
            break


def get_geojson_pcs(bucket, gpd_tile, sample_img_nm, img_fullpth_catalog, logger):
    """
    covert geojson of a tile to pcs of sample image.
    arg:
        bucket: Name of the S3 bucket.
        gpd_tile: geopandas object
        tile_folder: the folder name for storing tile geojson
        sample_img_nm: the name of sample image used to extract pcs
        logger: logger
    return:
        'extent_geojson' sharply geojson object
        'proj' projection object
    """

    # read projection from sample planet image
    #sub_img_pth = get_matching_s3_keys(bucket, prefix=prefix_x, suffix="{}_3B_AnalyticMS_SR.tif".format(sample_img_nm))
    s = img_fullpth_catalog.stack()  # convert entire data frame into a series of values
    sub_img_pth = img_fullpth_catalog.iloc[s[s.str.contains(sample_img_nm,na=False)].index.get_level_values(0)].values[0][0]
    uri_img_gdal = "/vsis3/{}/{}".format(bucket, sub_img_pth)
    img = gdal.Open(uri_img_gdal)
    if img is None:
        logger.error("reading {} failed". format(uri_img_gdal))

    # convert tile to planet image pcs
    gpd_tile_pcs = gpd_tile.to_crs(epsg=osr.SpatialReference(wkt=img.GetProjection()).GetAttrValue('AUTHORITY',1))
    extent_geojson = mapping(gpd_tile_pcs['geometry'])
    proj = img.GetProjectionRef()
    img = None
    return extent_geojson, proj


def get_extent(extent_geojson, res):
    """
    read geojson of a tile from an S3 bucket, and convert projection to be aligned with sample image.
    arg:
        'extent_geojson': sharply geojson object
        res: planet resolution
    return:
        (float, float, float, float), (int, int)) tuple
    """
    # txmin = min([row[0] for row in extent_geojson['coordinates'][0]]) - res / 2.0
    # txmax = max([row[0] for row in extent_geojson['coordinates'][0]]) + res / 2.0
    # tymin = min([row[1] for row in extent_geojson['coordinates'][0]]) - res / 2.0
    # tymax = max([row[1] for row in extent_geojson['coordinates'][0]]) + res / 2.0
    txmin = extent_geojson['bbox'][0] - res * 20
    txmax = extent_geojson['bbox'][2] + res * 20
    tymin = extent_geojson['bbox'][1] - res * 20
    tymax = extent_geojson['bbox'][3] + res * 20
    n_row = ceil((tymax - tymin)/res)
    n_col = ceil((txmax - txmin)/res)
    txmin_new = (txmin + txmax)/2 - n_row / 2 * res
    txmax_new = (txmin + txmax)/2 + n_row / 2 * res
    tymin_new = (tymin + tymax)/2 - n_col / 2 * res
    tymax_new = (tymin + tymax)/2 + n_col / 2 * res
    return (txmin_new, txmax_new, tymin_new, tymax_new), (n_row, n_col)


def parse_yaml_from_s3(bucket, prefix):
    """
    read bucket, prefix from yaml.
    arg:
        bucket: Name of the S3 bucket.
        prefix: the name for yaml file
    return:
        yaml object
    """
    s3 = boto3.resource('s3')
    obj = s3.Bucket(bucket).Object(prefix).get()['Body'].read()
    return yaml.load(obj)


def parse_catalog_from_s3(bucket, prefix, catalog_name):
    """
    read bucket, prefix from yaml.
    arg:
        bucket: Name of the S3 bucket.
        prefix: prefix for yaml file
        catalog_name: name of catalog file
    return:
        'catalog' pandas object
    """
    s3 = boto3.client('s3')
    obj = s3.get_object(Bucket=bucket, Key='{}/{}'.format(prefix, catalog_name))
    catalog = pd.read_csv(obj['Body'], sep=" ")
    return catalog


def delete_file(file_pth, logger):
    """
    delete the file given a specific path
    arg:
        file_pth: full path of file to delete.
        logger: handler of logging file
    """
    try:
        os.remove(file_pth)
    except OSError as e:
        logger.warning("Removing {} fails: {} ".format(file_pth, e.strerror))


def run_cmd(cmd, logger):
    """
    using os to run a command line
    arg:
        cmd: a command line
        logger: handler of logging file
    """
    try:
        os.system(cmd)
    except OSError as e:
        logger.error("Runing command line '{}' fails: {}".format(cmd, e))
        raise


def is_valid_image(path, logger):
    ds = gdal.Open(path)
    if ds is None:
        return False
    
    rasterArray = np.array(ds.GetRasterBand(1).ReadAsArray())
    unique_val = np.unique(rasterArray)
    if len(unique_val) == 1:
        del ds
        return False
    else:
        del ds
        return True


def ard_generation(sub_catalog, img_fullpth_catalog, bucket, tile_id, proj, bounds, n_row, n_col, tmp_pth, logger,
                   dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal):
    """
    generate ARD image per image in sub catalog.
    arg:
        sub_catalog: a list recording all images for the focused tile_ids
        img_fullpth_catalog: a catalog for recording full uri path for each planet image
        bucket: Name of the S3 bucket.
        tile_id: id of current tile to be processed
        proj: the projection of outputted ARD
        bounds: xmin, xmax, ymin, ymax defining the extent of ard
        tmp_path: tmp path defining the path for storing temporal files
        logger: logging object
        dry_lower_ordinal: lower bounds of ordinal days for dry season
        dry_upper_ordinal: upper bounds of ordinal days for dry season
        wet_lower_ordinal: lower bounds of ordinal days for wet season
        wet_upper_ordinal: upper bounds of ordinal days for wet season
    return:
        'extent_geojson' sharply geojson object
    """
    # initialize a record list for clear observations for each days
    clear_records = [0] * 366
    imgname_records = [0] * 366

    # local_tile_folder: tmp folder for saving ard in the instance
    local_tile_folder = os.path.join(tmp_pth, 'tile{}'.format(tile_id))
    if not os.path.exists(local_tile_folder):
        os.mkdir(local_tile_folder)

    # convert entire data frame into a series of values
    s = img_fullpth_catalog.stack()

    # iterate over each planet image for focused tile_id
    for i in range(len(sub_catalog)):
        img_name = sub_catalog.iloc[i,0]
        # sub_img_name = get_matching_s3_keys(bucket, prefix=prefix_x, suffix="{}_3B_AnalyticMS_SR.tif".format(img_name))
        single_img_pth = img_fullpth_catalog.iloc[s[s.str.contains(img_name,na=False)].index.get_level_values(0)].values[0][0]
        if single_img_pth is None:
            continue
        single_msk_pth = single_img_pth.replace('AnalyticMS_SR', 'AnalyticMS_DN_udm')

        # note that gdal and rasterio uri formats are different
        uri_img_gdal = "/vsis3/{}/{}".format(bucket, single_img_pth)
        uri_msk_gdal = "/vsis3/{}/{}".format(bucket, single_msk_pth)

        ordinal_dates = datetime.strptime(img_name[0:8], '%Y%m%d').date().toordinal()
        if ordinal_dates not in range(dry_lower_ordinal, dry_upper_ordinal + 1) and ordinal_dates \
                not in range(wet_lower_ordinal, wet_upper_ordinal + 1):
            continue

        doy = datetime.strptime(img_name[0:8], '%Y%m%d').date().timetuple().tm_yday

        outname = "PLANET%s%s" % (str(datetime.strptime(img_name[0:8], '%Y%m%d').date().year),
                                  str("{0:0=3d}".format(doy)))

        img = gdal.Open(uri_img_gdal)
        msk = gdal.Open(uri_msk_gdal)

        # img or msk is missing, give a warning and then skip
        if img is None:
            logger.warning("couldn't find {} from s3 for tile {}".format(uri_img_gdal, tile_id))
            continue

        if msk is None:
            logger.warning("couldn't find {} from s3 for tile {}".format(uri_msk_gdal, tile_id))
            continue

        out_img = gdal.Warp(os.path.join(local_tile_folder, '_tmp_img'), img, outputBounds=[bounds[0], bounds[2], bounds[1],
                                                                                  bounds[3]],
                            width=n_col, height=n_row, dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS=proj)
        out_msk = gdal.Warp(os.path.join(local_tile_folder, '_tmp_msk'), msk, outputBounds=[bounds[0], bounds[2], bounds[1],
                                                                                  bounds[3]], width=n_col, height=n_row,
                            dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS=proj)

        if out_img is None:
            logger.warning("Running gdal.Warp fails for {} for tile {}".format(uri_img_gdal, tile_id))
            continue

        if out_msk is None:
            logger.warning("Running gdal.Warp fails for {} for tile {}".format(uri_msk_gdal, tile_id))
            continue

        n_valid_pixels = len(out_img.GetRasterBand(1).ReadAsArray()[out_img.GetRasterBand(1).ReadAsArray() > -9999])
        n_clear_pixels = len(out_msk.GetRasterBand(1).ReadAsArray()[out_msk.GetRasterBand(1).ReadAsArray() == 0])

        # firstly, see if clear observation is more than the record; if not, not necessary to process
        if n_clear_pixels < clear_records[doy-1]:
            continue
        else:
            if n_clear_pixels/n_valid_pixels > 0.2:
                # if already created, delete old files
                if clear_records[doy-1] > 0:
                    os.remove(os.path.join(local_tile_folder, imgname_records[doy-1]))
                    os.remove(os.path.join(local_tile_folder, imgname_records[doy-1]+'.hdr'))

                out_img_b1_med = ndimage.median_filter(out_img.GetRasterBand(1).ReadAsArray(), size=3)
                out_img_b2_med = ndimage.median_filter(out_img.GetRasterBand(2).ReadAsArray(), size=3)
                out_img_b3_med = ndimage.median_filter(out_img.GetRasterBand(3).ReadAsArray(), size=3)
                out_img_b4_med = ndimage.median_filter(out_img.GetRasterBand(4).ReadAsArray(), size=3)
        
            
                clear_records[doy-1] = n_clear_pixels
                imgname_records[doy-1] = outname + img_name[8:len(img_name)]
                outdriver1 = gdal.GetDriverByName("ENVI")
                outdata = outdriver1.Create(os.path.join(local_tile_folder,
                                                    outname+img_name[8:len(img_name)]),
                                            n_col, n_row, 5, gdal.GDT_Int16, options=["INTERLEAVE=BIP"])
                outdata.GetRasterBand(1).WriteArray(out_img_b1_med)
                outdata.FlushCache()
                outdata.GetRasterBand(2).WriteArray(out_img_b2_med)
                outdata.FlushCache()
                outdata.GetRasterBand(3).WriteArray(out_img_b3_med)
                outdata.FlushCache()
                outdata.GetRasterBand(4).WriteArray(out_img_b4_med)
                outdata.FlushCache()
                outdata.GetRasterBand(5).WriteArray(out_msk.GetRasterBand(1).ReadAsArray())
                outdata.FlushCache()

                outdata.SetGeoTransform(out_img.GetGeoTransform())
                outdata.FlushCache()
                outdata.SetProjection(proj)
                outdata.FlushCache()

                del outdata

        del img
        del msk

        del out_img
        del out_msk

    # delete tmp image and mask
    delete_file(os.path.join(local_tile_folder, '_tmp_img'), logger)
    delete_file(os.path.join(local_tile_folder, '_tmp_msk'), logger)


def composite_generation(compositing_exe_path, bucket, prefix, foc_gpd_tile, tile_id, ard_folder, tmp_pth,
                         logger, dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, 
                         wet_upper_ordinal, bsave_ard, output_prefix):
    """
    generate composite image in sub catalog.
    arg:
        compositing_exe_path: directory for compositing exe
        bucket: Name of the S3 bucket.
        prefix: the prefix for tile_folder and img_folder
        foc_gpd_tile: geopandas object indicating the extent of a focused tile, using GCS system
        tile_id: id of current tile to be processed
        ard_folder: the folder name for storing ard images
        tmp_pth: the outputted folder for pcs and gcs images
        logger: logging object
        dry_lower_ordinal: lower bounds of ordinal days for dry season
        dry_upper_ordinal: upper bounds of ordinal days for dry season
        wet_lower_ordinal: lower bounds of ordinal days for wet season
        wet_upper_ordinal: upper bounds of ordinal days for wet season
        bsave_ard: if save ard images
        output_prefix: prefix for composite in S3 
    """


    # fetch tile info, which will be used to crop intermediate compositing image
    extent_geojson_gcs = mapping(foc_gpd_tile['geometry'])
    txmin = extent_geojson_gcs['bbox'][0]
    txmax = extent_geojson_gcs['bbox'][2]
    tymin = extent_geojson_gcs['bbox'][1]
    tymax = extent_geojson_gcs['bbox'][3]

    #######################################################
    #           1. begin compositing dryseason            #
    #######################################################
    cmd = [compositing_exe_path, ard_folder, tmp_pth, str(tile_id), str(dry_lower_ordinal), str(dry_upper_ordinal)]
    # run composite exe
    try:
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        logger.error("compositing error for tile {} at dry season: {}".format(tile_id, e))
        raise


    # reproject and crop compositing image to align with GCS tile system
    out_path_pcs_dry = os.path.join(tmp_pth, 'tile{}_{}_{}_pcs.tif'.format(tile_id, dry_lower_ordinal, dry_upper_ordinal))
    out_path_gcs_dry = os.path.join(tmp_pth, 'tile{}_{}_{}_gcs.tif'.format(tile_id, dry_lower_ordinal, dry_upper_ordinal))
    out_path_gcs_dry_TCI = os.path.join(tmp_pth, 'tile{}_{}_{}_gcs_TCI.tif'.format(tile_id, dry_lower_ordinal, dry_upper_ordinal))
    out_path_dry = os.path.join(tmp_pth, 'tile{}_{}_{}.tif'.format(tile_id, dry_lower_ordinal, dry_upper_ordinal))

    # check if composite image is valid
    if not is_valid_image(out_path_pcs_dry, logger):
        logger.info("Composition fails for the first time for tile{}_{}_{} at dry season".format(tile_id, dry_lower_ordinal, dry_upper_ordinal))
        # run composite exe
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)

        if not is_valid_image(out_path_pcs_dry, logger):
            logger.info("composition fails for tile {} for twice at dry season".format(tile_id))
            p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
            if not is_valid_image(out_path_pcs_dry, logger):
                logger.error("compositing error for tile {} for the third time at dry season".format(tile_id))
                raise FuncException("Composition fails")


    # here call gdalwarp directly instead of gdal.warp, cause unexpected bug for gdal.warp
    # out_img = gdal.Warp(out_path_gcs_dry, img, outputBounds=[txmin, tymin, txmax, tymax], resampleAlg=gdal.GRA_Bilinear, width=2000,
                        # height=2000, dstNodata=-9999,outputType=gdal.GDT_Int16, dstSRS='EPSG:4326')
    cmd = 'gdalwarp -q -overwrite -t_srs EPSG:4326 -te {} {} {} {} -r bilinear -ts 2000 2000 -srcnodata -9999 -dstnodata -9999 -ot ' \
          'Int16 {} {}'.format(txmin, tymin, txmax, tymax, out_path_pcs_dry, out_path_gcs_dry)

    run_cmd(cmd, logger)
    

    ######################################################################################eo
    #                 convert to Cloud-Optimized Geotiff                                  #
    #        (source: https://trac.osgeo.org/gdal/wiki/CloudOptimizedGeoTIFF)             #
    #   why create a memory driver filer, not directly created:                           #
    #   the problem is that this will give an error in the COG format, because the        # 
    #   pyramids were created after the tiling.                                           #
    #######################################################################################
    cmd = 'gdal_translate -q {} {} -co COMPRESS=LZW -co TILED=YES'. format(out_path_gcs_dry, out_path_gcs_dry_TCI)
    run_cmd(cmd, logger)

    cmd = 'gdaladdo -q -r average {} 2 4'. format(out_path_gcs_dry_TCI)
    run_cmd(cmd, logger)

    cmd = 'gdal_translate -q {} {} -co COMPRESS=LZW -co COPY_SRC_OVERVIEWS=YES -co TILED=YES'.format(out_path_gcs_dry_TCI,
                                                                                                     out_path_dry)
    run_cmd(cmd, logger)


    #########################################################
    #            2. begin compositing wet season            #
    #########################################################
    cmd = [compositing_exe_path, ard_folder,tmp_pth, str(tile_id), str(wet_lower_ordinal), str(wet_upper_ordinal)]
    # run composite exe
    try:
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        logger.error("compositing error for tile {} at wet season: {}".format(tile_id, e))
        raise


    # reproject and crop compositing image to align with GCS tile system         
    out_path_pcs_wet = os.path.join(tmp_pth, 'tile{}_{}_{}_pcs.tif'.format(tile_id, wet_lower_ordinal, wet_upper_ordinal))
    out_path_gcs_wet = os.path.join(tmp_pth, 'tile{}_{}_{}_gcs.tif'.format(tile_id, wet_lower_ordinal, wet_upper_ordinal))
    out_path_gcs_wet_TCI = os.path.join(tmp_pth, 'tile{}_{}_{}_gcs_TCI.tif'.format(tile_id, wet_lower_ordinal, wet_upper_ordinal))
    out_path_wet = os.path.join(tmp_pth, 'tile{}_{}_{}.tif'.format(tile_id, wet_lower_ordinal, wet_upper_ordinal))
        

    # check if composite image is valid
    if not is_valid_image(out_path_pcs_wet, logger):
        logger.info("Composition fails for the first time for tile{}_{}_{} at wet season".format(tile_id, wet_lower_ordinal, wet_upper_ordinal))
        # run composite exe
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)

        if not is_valid_image(out_path_pcs_wet, logger):
            logger.info("composition fail for twice for tile {} at wet season".format(tile_id))
            p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
            if not is_valid_image(out_path_pcs_wet, logger):
                logger.error("composition fail for the third time for tile {} at wet season".format(tile_id))
                raise FuncException("Composition fails")

    # img = gdal.Open(out_path_pcs_wet)
    # if img is None:
        # logger.error("couldn't find pcs-based compositing result for tile {}".format(tile_id))
        # return

    # out_img = gdal.Warp(out_path_gcs_wet, img, outputBounds=[txmin, tymin, txmax, tymax], resampleAlg=gdal.GRA_Bilinear, width=2000,
                        #height=2000, dstNodata=-9999, xRes=0.05/2000, yRes=0.05/2000, outputType=gdal.GDT_Int16, dstSRS='EPSG:4326')

    cmd = 'gdalwarp -q -overwrite -t_srs EPSG:4326 -te {} {} {} {} -r bilinear -ts 2000 2000 -srcnodata -9999 -dstnodata -9999 -ot ' \
          'Int16 {} {}'.format(txmin, tymin, txmax, tymax, out_path_pcs_wet, out_path_gcs_wet)
    run_cmd(cmd, logger)


    # convert to Cloud-Optimized Geotiff          
    cmd = 'gdal_translate -q {} {} -co COMPRESS=LZW -co TILED=YES'. format(out_path_gcs_wet, out_path_gcs_wet_TCI)
    run_cmd(cmd, logger)

    cmd = 'gdaladdo -q -r average {} 2 4'. format(out_path_gcs_wet_TCI)
    run_cmd(cmd, logger)

    cmd = 'gdal_translate -q {} {} -co COMPRESS=LZW -co COPY_SRC_OVERVIEWS=YES -co TILED=YES'. format(out_path_gcs_wet_TCI,
                                                                                                      out_path_wet)
    run_cmd(cmd, logger)


    ############################################################
    #             3.upload compositing image to s3             #
    ############################################################
    s3 = boto3.client('s3')
    try:
        s3.upload_file(out_path_dry, bucket, '{}/{}/OS/tile{}_{}_{}.tif'.format(prefix, output_prefix, tile_id, dry_lower_ordinal,
                                                                                dry_upper_ordinal))
    except ClientError as e:
        logger.error("S3 uploading fails for tile{}_{}_{} : {}".format(tile_id, dry_lower_ordinal, dry_upper_ordinal, e))
        raise

    try:
        s3.upload_file(out_path_wet, bucket, '{}/{}/GS/tile{}_{}_{}.tif'.format(prefix, output_prefix, tile_id, wet_lower_ordinal,
                                                                                wet_upper_ordinal))
    except ClientError as e:
        logger.error("S3 uploading fails for tile{}_{}_{}: {}".format(tile_id, wet_lower_ordinal, wet_upper_ordinal, e))
        raise

    ##########################################################
    #             delete local composte files                #
    ##########################################################

    # ARD folder
    if bsave_ard is False:
        # dry season
        delete_file(out_path_gcs_dry, logger)
        delete_file(out_path_pcs_dry, logger)
        delete_file(out_path_gcs_dry_TCI, logger)
        delete_file(out_path_dry, logger)

        # wet season
        delete_file(out_path_gcs_wet, logger)
        delete_file(out_path_pcs_wet, logger)
        delete_file(out_path_gcs_wet_TCI, logger)
        delete_file(out_path_wet, logger)

        # Try to delete the ard image folder
        try:
            shutil.rmtree(ard_folder)
        except OSError as e:  # if failed, report it back to the user ##
            logger.warning("Error for removing ARD folder {}: {} ".format(ard_folder, e.strerror))


def ard_composition_execution(foc_img_catalog, foc_gpd_tile, tile_id, s3_bucket, prefix, img_fullpth_catalog, tmp_pth, compositing_exe_path,
                              dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal, bsave_ard, output_prefix,
                              res, logger, total_number, failure_count):
    """
    executing ARD and composite generation.
    arg:
        foc_img_catalog: a list recording all images for a focused tile id
        foc_gpd_tile: geopandas object indicating the extent of a focused tile, using GCS system
        tile_id: id of tile to be focused
        s3_bucket: Name of the S3 bucket.
        prefix: the prefix for tile_folder and img_folder
        img_fullpth_catalog: a catalog for recording full uri path for each planet image
        tmp_path: tmp path defining the path for storing temporal files
        compositing_exe_path: directory for compositing exe
        dry_lower_ordinal: lower bounds of ordinal days for dry season
        dry_upper_ordinal: upper bounds of ordinal days for dry season
        wet_lower_ordinal: lower bounds of ordinal days for wet season
        wet_upper_ordinal: upper bounds of ordinal days for wet season
        bsave_ard: if save ard images
        output_prefix: prefix for composite in S3
        res: resolution
        logger: logging object
        total_number: finished tile number for progress report
    """

    # read proj and bounds from the first img of aoi
    sample_img_nm = foc_img_catalog.iloc[0, 0]
    tile_geojson, proj = get_geojson_pcs(s3_bucket, foc_gpd_tile, sample_img_nm, img_fullpth_catalog, logger)
    bounds, (n_row, n_col) = get_extent(tile_geojson, res)

    # generate ARD images
    ard_generation(foc_img_catalog, img_fullpth_catalog, s3_bucket, tile_id, proj, bounds, n_row, n_col,
                   tmp_pth, logger, dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal)

    # compositing
    try:
        composite_generation(compositing_exe_path, s3_bucket, prefix, foc_gpd_tile, tile_id,
                             os.path.join(tmp_pth, 'tile{}'.format(tile_id)), tmp_pth, logger, dry_lower_ordinal,
                             dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal, bsave_ard, output_prefix)
    except (OSError, ClientError, subprocess.CalledProcessError, FuncException) as e:
        failure_count = failure_count + 1
        logger.error("Compositing failed for tile_id {} ; the total failure count is {} ({})".format(tile_id, failure_count, datetime.now(timezone('US/Eastern'))
                                                                      .strftime('%Y-%m-%d %H:%M:%S')))
    else:
        logger.info("Progress: finished compositing for tile_id {} ({}))".format(tile_id, datetime.now(timezone('US/Eastern'))
                                                                                 .strftime('%Y-%m-%d %H:%M:%S')))
        total_number = total_number + 1

    


@click.command()
@click.option('--config_filename', default='cvmapper_config_composite.yaml', help='The name of the config to use.')
@click.option('--tile_id', default=None, help='only used for debug mode, user-defined tile_id')
@click.option('--aoi', default=None, help='specify AOI id to work on')
@click.option('--csv_pth', default=None, help='csv path for providing a specified tile list')
@click.option('--bsave_ard', default=False, help='only used for debug mode, user-defined tile_id')
@click.option('--s3_bucket', default='activemapper',help='s3 bucket name')
@click.option('--output_prefix', default='composite_sr', help='output folder prefix')
@click.option('--threads_number', default='default', help='output folder prefix')
def main(s3_bucket, config_filename, tile_id, aoi, csv_pth, bsave_ard, output_prefix, threads_number):
    """ The primary script
        Args:        
        s3_bucket (str): Name of the S3 bucket to search for configuration objects
            and save results to
        config_filename: configuration file name
        tile_id(optional, only for testing stage)
    """

    # define res
    res = 3

    # define compositing ext
    compositing_exe_path = '/home/ubuntu/imager/C/AFMapTSComposite/bin/composite'

    # define log path
    log_path = '%s/log/planet_composite.log' % os.environ['HOME']
    logging.basicConfig(filename=log_path, filemode='w', level=logging.INFO)
    logger = logging.getLogger(__name__)

    tmp_pth = '/tmp'

    # parse mapper parameter from yaml
    params = parse_yaml_from_s3(s3_bucket, config_filename)['mapper']

    # read individual parameters
    prefix = params['prefix']

    # fetching a table linking  planet images name and tile id
    img_catalog_name = params['img_catalog_name']

    # fetching a table recording full pth of planet images
    img_catalog_pth = params['img_catalog_pth']

    # a geojson indicating the extent of each tile
    tiles_geojson_path = params['tile_geojson_path']

    # define lower and upper bounds for dry and wet season
    dry_lower_ordinal = params['dry_lower_ordinal'] # 2018/12/01
    dry_upper_ordinal = params['dry_upper_ordinal'] # 2019/02/28
    wet_lower_ordinal = params['wet_lower_ordinal'] # 2018/05/01
    wet_upper_ordinal = params['wet_upper_ordinal'] # 2018/09/30

    # time zone
    tz = timezone('US/Eastern')
    logger.info("Progress: starting a compositing task ({})".format(datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))

    # read a catalog for linking planet images and tile id
    img_catalog = parse_catalog_from_s3(s3_bucket, prefix, img_catalog_name)

    # read a catalog recording full path for planet images
    img_fullpth_catalog = parse_catalog_from_s3(s3_bucket, prefix, img_catalog_pth)

    # read a geopandas object for tile geojson
    uri_tile = "s3://{}/{}/{}".format(s3_bucket, prefix, tiles_geojson_path)
    gpd_tile = gpd.read_file(uri_tile)
    if gpd_tile is None:
        logger.error("reading geojson tile '{}' failed". format(uri_tile))

    if tile_id is None:
        # determine thread number to be used
        if threads_number == 'default':
            threads_number = multiprocessing.cpu_count() * 2
        else:
            threads_number = int(threads_number)

        ard_composition_executor = FixedThreadPoolExecutor(size=threads_number)

        if aoi is None:
            if csv_pth is None:
                logger.error("Please provide tile_id, csv_path or aoi_id ({}))"
                             .format(datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))
                return

            # tile-list processing(for production)
            aoi_alltiles = pd.read_csv(csv_pth)['tile']
        else:

            # aoi-based processing(for production)
            aoi_alltiles = gpd_tile.loc[gpd_tile['aoi'] == float(aoi)]['tile']
        
        failure_count = 0
        success_count = 0
        # looping over each tile
        for i in range(len(aoi_alltiles)):

            # retrive all tile info for focused tile_id
            tile_id = int(aoi_alltiles.iloc[i])
            foc_img_catalog = img_catalog.loc[img_catalog['tile'] == tile_id]
            foc_gpd_tile = gpd_tile[gpd_tile['tile'] == int(tile_id)]

            ard_composition_executor.submit(ard_composition_execution, foc_img_catalog, foc_gpd_tile, tile_id, s3_bucket,
                                            prefix, img_fullpth_catalog, tmp_pth, compositing_exe_path, dry_lower_ordinal,
                                            dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal, bsave_ard, output_prefix,
                                            res, logger, success_count, failure_count)


        # await all tile finished
        ard_composition_executor.drain()

        # await threadpool to stop
        ard_composition_executor.close()

        logger.info("Progress: finished compositing task for aoi {}; the total tile number to be processed is {}; the success_count is {}; the failure_count is {} ({})"
                    .format(aoi, len(aoi_alltiles), success_count, failure_count, datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))

    # tile-based processing (for debug)
    else:
        # fetch all planet image relating to focused tile id
        foc_img_catalog = img_catalog.loc[img_catalog['tile'] == int(tile_id)]
        sample_img_nm = foc_img_catalog.iloc[0, 0]
        foc_gpd_tile = gpd_tile[gpd_tile['tile'] == int(tile_id)]
        tile_geojson, proj = get_geojson_pcs(s3_bucket, foc_gpd_tile, sample_img_nm, img_fullpth_catalog, logger)
        bounds, (n_row, n_col) = get_extent(tile_geojson, res)

        # ARD generation
        try:
            ard_generation(foc_img_catalog, img_fullpth_catalog, s3_bucket,  int(tile_id),  proj, bounds, n_row, n_col, tmp_pth,
                           logger, dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal)
        except:
            logger.error("ARD generation failed for tile_id {}, and the total finished tiles is {} ({}))"
                         .format(tile_id,  1, datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))
        else:
            logger.info("Progress: finished ARD generation for tile_id {}, and the total finished tiles is {} ({}))"
                        .format(tile_id,  1, datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))

        # compositing
        try:
            composite_generation(compositing_exe_path, s3_bucket, prefix,  foc_gpd_tile, tile_id,
                                 os.path.join(tmp_pth, 'tile{}'.format(tile_id)), tmp_pth, logger, dry_lower_ordinal,
                                 dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal, bsave_ard, output_prefix)
        except (OSError, ClientError, subprocess.CalledProcessError) as e:
            logger.error("Compositing failed for tile_id {}, and the total finished tiles is {} ({}))"
                         .format(tile_id,  1, datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))

        else:
            logger.info("Progress: finished compositing for tile_id {}, and the total finished tiles is {} ({}))"
                        .format(tile_id,  1, datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')))

if __name__ == '__main__':
    main()

