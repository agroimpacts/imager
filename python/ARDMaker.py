"""
This module is developed to make ARD and call AFMapTSComposite for making composites
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


def get_geojson_pcs(bucket, gjs_tile, prefix,  img_folder, sample_img_nm, logger):
    """
    covert geojson of a tile to pcs of sample image.
    arg:
        bucket: Name of the S3 bucket.
        gjs_tile: geopandas object
        prefix: the prefix for tile_folder and img_folder
        tile_folder: the folder name for storing tile geojson
        img_folder: the folder name for storing image geojson
        sample_img_nm: the name of sample image used to extract pcs
        logger: logger
    return:
        'extent_geojson' sharply geojson object
        'proj' projection object
    """

    # read projection from sample planet image
    prefix_x = "{}/{}".format(prefix, img_folder)
    sub_img_name = get_matching_s3_keys(bucket, prefix=prefix_x, suffix="{}_3B_AnalyticMS_SR.tif".format(sample_img_nm))
    uri_img_gdal = "/vsis3/{}/{}".format(bucket, sub_img_name)
    img = gdal.Open(uri_img_gdal)
    if img is None:
        logger.error("reading {} failed". format(uri_img_gdal))

    # convert tile to planet image pcs
    gjs_tile_pcs = gjs_tile.to_crs(epsg=osr.SpatialReference(wkt=img.GetProjection()).GetAttrValue('AUTHORITY',1))
    extent_geojson = mapping(gjs_tile_pcs['geometry'][0])
    proj = img.GetProjectionRef()
    img = None
    return extent_geojson, proj


def get_extent(extent_geojson, res):
    """
    read geojson of a tile from an S3 bucket, and convert projection to sample image.
    arg:
        'extent_geojson': sharply geojson object
        res: planet resolution
    return:
        (float, float, float, float, int, int) tuple
    """
    txmin = min([row[0] for row in extent_geojson['coordinates'][0]]) - res / 2.0
    txmax = max([row[0] for row in extent_geojson['coordinates'][0]]) + res / 2.0
    tymin = min([row[1] for row in extent_geojson['coordinates'][0]]) - res / 2.0
    tymax = max([row[1] for row in extent_geojson['coordinates'][0]]) + res / 2.0
    n_row = ceil((tymax - tymin)/res)
    n_col = ceil((txmax - txmin)/res)
    txmin_new = (txmin + txmax)/2 - n_row / 2 * res
    txmax_new = (txmin + txmax)/2 + n_row / 2 * res
    tymin_new = (tymin + tymax)/2 - n_col / 2 * res
    tymax_new = (tymin + tymax)/2 + n_col / 2 * res
    return txmin_new, txmax_new, tymin_new, tymax_new, n_row, n_col


def parse_yaml_from_s3(bucket, prefix):
    """
    read bucket, prefix from yaml.
    arg:
        bucket: Name of the S3 bucket.
        prefix: the prefix for yaml folder
    return:
        'extent_geojson' sharply geojson object
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


def ard_generation(sub_catalog, bucket, prefix, aoi, tile_id, img_folder, proj, bounds, tmp_pth, logger,
                   dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal):
    """
    generate ARD image per image in sub catalog.
    arg:
        sub_catalog: a table recording all images for tile_id
        bucket: Name of the S3 bucket.
        prefix: the prefix for tile_folder and img_folder
        aoi: aoi to be processed
        tile_id: id of current tile to be processed
        img_folder: the folder name for storing image geojson
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
    n_row = bounds[4]
    n_col = bounds[5]

    if not os.path.exists('/tmp/aoi{}_tile{}'.format(aoi, tile_id)):
        os.mkdir('/{}/aoi{}_tile{}'.format(tmp_pth, aoi, tile_id))

    ###############################################
    for i in range(len(sub_catalog)):
        img_name = sub_catalog.iloc[i,0]
        prefix_x = "{}/{}".format(prefix, img_folder)
        sub_img_name = get_matching_s3_keys(bucket, prefix=prefix_x, suffix="{}_3B_AnalyticMS_SR.tif".format(img_name))
        if sub_img_name is None:
            continue
        sub_msk_name = sub_img_name.replace('AnalyticMS_SR', 'AnalyticMS_DN_udm')

        # note that gdal and rasterio uri formats are different
        # uri_img = "s3://{}/{}".format(bucket, sub_img_name)
        uri_img_gdal = "/vsis3/{}/{}".format(bucket, sub_img_name)
        # uri_msk = "s3://{}/{}".format(bucket, sub_msk_name)
        uri_msk_gdal = "/vsis3/{}/{}".format(bucket, sub_msk_name)

        ordinal_dates = datetime.strptime(img_name[0:8], '%Y%m%d').date().toordinal()
        if ordinal_dates not in range(dry_lower_ordinal, dry_upper_ordinal + 1) and ordinal_dates \
                not in range(wet_lower_ordinal, wet_upper_ordinal + 1):
            continue

        doy = datetime.strptime(img_name[0:8], '%Y%m%d').date().timetuple().tm_yday

        outname = "PLANET%s%s" % (str(datetime.strptime(img_name[0:8], '%Y%m%d').date().year),
                                  str("{0:0=3d}".format(doy)))

        img = gdal.Open(uri_img_gdal)
        msk = gdal.Open(uri_msk_gdal)

        # partial dataset, skip
        if img is None:
            logger.warn("couldn't find {} from s3".format(uri_img_gdal))
            continue

        if msk is None:
            logger.warn("couldn't find {} from s3".format(uri_msk_gdal))
            continue

        out_img = gdal.Warp(os.path.join(tmp_pth, '_tmp_img'), img, outputBounds=[bounds[0], bounds[2], bounds[1],
                                                                                  bounds[3]],
                            width=n_col, height=n_row, dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS=proj)
        out_msk = gdal.Warp(os.path.join(tmp_pth, '_tmp_msk'), msk, outputBounds=[bounds[0], bounds[2], bounds[1],
                                                                                  bounds[3]], width=n_col, height=n_row,
                            dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS=proj)

        n_valid_pixels = len(out_img.GetRasterBand(1).ReadAsArray()[out_img.GetRasterBand(1).ReadAsArray() > -9999])
        n_clear_pixels = len(out_msk.GetRasterBand(1).ReadAsArray()[out_msk.GetRasterBand(1).ReadAsArray() == 0])

        # firstly, see if clear observation is more than the record; if not, not necessary to process
        if n_clear_pixels < clear_records[doy-1]:
            continue
        else:
            if n_clear_pixels/n_valid_pixels > 0.2:

                # if already created, delete old files
                if clear_records[doy-1] > 0:
                    os.remove(os.path.join(tmp_pth, imgname_records[doy-1]))
                    os.remove(os.path.join(tmp_pth, imgname_records[doy-1]+'.hdr'))

                out_img_b1_med = ndimage.median_filter(out_img.GetRasterBand(1).ReadAsArray(), size=3)
                out_img_b2_med = ndimage.median_filter(out_img.GetRasterBand(2).ReadAsArray(), size=3)
                out_img_b3_med = ndimage.median_filter(out_img.GetRasterBand(3).ReadAsArray(), size=3)
                out_img_b4_med = ndimage.median_filter(out_img.GetRasterBand(4).ReadAsArray(), size=3)

                clear_records[doy-1] = n_clear_pixels
                imgname_records[doy-1] = outname + img_name[8:len(img_name)]
                outdriver1 = gdal.GetDriverByName("ENVI")
                outdata = outdriver1.Create(os.path.join('/{}/aoi{}_tile{}'.format(tmp_pth, aoi, tile_id),
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

        img = None
        msk = None

    # delete tmp image and mask
    os.remove(os.path.join(tmp_pth, '_tmp_img'))
    os.remove(os.path.join(tmp_pth, '_tmp_msk'))


def composite_generation(compositing_exe_path, bucket, prefix, country_name, gjs_tile, tile_id, img_folder, out_folder,
                         logger, dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal):
    """
    generate composite image in sub catalog.
    arg:
        compositing_exe_path: directory for compositing exe
        gjs_tile: geopandas vector file for a targeted tile, using GCS system
        tile_id: id of current tile to be processed
        img_folder: the folder name for storing image geojson
        out_folder: the outputted folder for pcs and gcs images
        logger: logging object
        dry_lower_ordinal: lower bounds of ordinal days for dry season
        dry_upper_ordinal: upper bounds of ordinal days for dry season
        wet_lower_ordinal: lower bounds of ordinal days for wet season
        wet_upper_ordinal: upper bounds of ordinal days for wet season
    """

    # fetch tile info, which will be used to crop intermediate compositing image
    extent_geojson_gcs = mapping(gjs_tile['geometry'][0])
    txmin = min([row[0] for row in extent_geojson_gcs['coordinates'][0]])
    txmax = max([row[0] for row in extent_geojson_gcs['coordinates'][0]])
    tymin = min([row[1] for row in extent_geojson_gcs['coordinates'][0]])
    tymax = max([row[1] for row in extent_geojson_gcs['coordinates'][0]])

    # compositing dry season
    out_path_pcs_dry = os.path.join(out_folder, 'tile{}_{}_{}_pcs.tif'.format(tile_id, dry_lower_ordinal, dry_lower_ordinal))
    cmd = [compositing_exe_path, img_folder, out_path_pcs_dry, tile_id, dry_lower_ordinal, dry_upper_ordinal]
    # run composite exe
    try:
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        logger.error("compositing error for tile {} at dry season: {}".format(tile_id, e))

    # reproject and crop compositing image to align with GCS tile system
    out_path_gcs_dry = os.path.join(out_folder, 'tile{}_{}_{}.tif'.format(tile_id, dry_lower_ordinal, dry_lower_ordinal))
    img = gdal.Open(out_path_pcs_dry)
    if img is None:
        logger.error("couldn't find pcs-based compositing result for tile {}".format(tile_id))

    out_img = gdal.Warp(out_path_gcs_dry, img, outputBounds=[txmin, tymin, txmax, tymax], resampleAlg=gdal.GRA_Bilinear, width=2000,
                        height=2000, dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS='EPSG:4326')

    # release memory
    out_img = None
    img = None

    # compositing wet season
    out_path_pcs_wet = os.path.join(out_folder, 'tile{}_{}_{}_pcs.tif'.format(tile_id, wet_lower_ordinal, wet_lower_ordinal))
    cmd = [compositing_exe_path, img_folder, out_path_pcs_wet, tile_id, wet_lower_ordinal, wet_upper_ordinal]
    # run composite exe
    try:
        p = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        logger.error("compositing error for tile {} at wet season: {}".format(tile_id, e))

    # reproject and crop compositing image to align with GCS tile system
    out_path_gcs_wet = os.path.join(out_folder, 'tile{}_{}_{}.tif'.format(tile_id, wet_lower_ordinal, wet_lower_ordinal))
    img = gdal.Open(out_path_pcs_wet)
    if img is None:
        logger.error("couldn't find pcs-based compositing result for tile {}".format(tile_id))

    out_img = gdal.Warp(out_path_gcs_wet, img, outputBounds=[txmin, tymin, txmax, tymax], resampleAlg=gdal.GRA_Bilinear, width=2000,
                        height=2000, dstNodata=-9999, outputType=gdal.GDT_Int16, dstSRS='EPSG:4326')

    out_img = None
    img = None


    # # upload compositing image to s3
    s3 = boto3.client('s3')
    s3.upload_file(out_path_gcs_dry, bucket, '{}/composite_sr/{}/OS/{}'.format(prefix, country_name, out_path_gcs_dry))
    s3.upload_file(out_path_gcs_wet, bucket, '{}/composite_sr/{}/GS/{}'.format(prefix, country_name, out_path_gcs_dry))

    # delete local composte files
    os.remove(os.path.join(out_folder, out_path_gcs_dry))
    os.remove(os.path.join(out_folder, out_path_pcs_dry))
    os.remove(os.path.join(out_folder, out_path_gcs_wet))
    os.remove(os.path.join(out_folder, out_path_pcs_wet))

    # # Try to delete the tmp folder
    # try:
    #     os.rmdir('/{}/aoi{}_tile{}'.format(tmp_pth, aoi, tile_id))
    # except OSError as e:  # if failed, report it back to the user ##
    #     print("Error for removing %s ." % ('/{}/aoi{}_tile{}'.format(tmp_pth, aoi, tile_id)))



@click.command()
@click.option('--config_filename', default='cvmapper_config.yaml', help='The name of the config to use.')
@click.option('--tile_id', default=None, help='only used for debug mode, user-defined tile_id')
@click.argument('s3_bucket')
def main(s3_bucket, config_filename, tile_id):
    """ The primary script
    Args:        s3_bucket (str): Name of the S3 bucket to search for configuration objects
            and save results to
        config_filename: configuration file name
        tile_id(optional, only for testing stage)
    """

    # define res
    res = 3

    # define log path
    log_path = '%s/log/makingARD.log' % os.environ['HOME']
    logging.basicConfig(filename=log_path, filemode='w', level=logging.INFO)
    logger = logging.getLogger(__name__)

    out_folder = '/tmp/'

    # parse mapper parameter from yaml
    params = parse_yaml_from_s3(s3_bucket, config_filename)['mapper']
    prefix = params['prefix']
    # img_catalog links images and tiles
    img_catalog_name = params['img_catalog_name']
    img_all_folder = params['img_all_folder']
    tiles_geojson_path = params['tiles_geojson_path']
    ard_folder = params['ard_folder']
    aoi = params['aoi']

    # define lower and upper bounds for dry and wet season
    # 2017/12/01
    dry_lower_ordinal = params['dry_lower_ordinal']
    # 2018/02/28
    dry_upper_ordinal = params['dry_upper_ordinal']
    # 2018/05/01
    wet_lower_ordinal = params['wet_lower_ordinal']
    # 2018/09/30
    wet_upper_ordinal = params['wet_upper_ordinal']

    logger.info("starting making planet ARD images: {} for aoi_{}".format(time.time(), aoi))

    # fetch suprtile catalog
    img_catalog = parse_catalog_from_s3(s3_bucket, prefix, img_catalog_name)
    uri_tile = "s3://{}/{}/{}".format(s3_bucket, prefix, tiles_geojson_path)
    gjs_tile = gpd.read_file(uri_tile)
    if gjs_tile is None:
        logger.error("reading {} failed". format(uri_tile))

    # aoi-based processing
    if tile_id is None:
        # fetch image id for aoi
        foctiles = gjs_tile.loc[gjs_tile['aoi'] == int(aoi)]['aoi']

        # looping over each foctiles
        for i in range(len(foctiles)):
            foc_img_catalog = img_catalog.loc[img_catalog['tile'] == foctiles.iloc[i]]
            sample_img_nm = foc_img_catalog.iloc[0, 0]
            tile_geojson, proj = get_geojson_pcs(s3_bucket, gjs_tile[gjs_tile['id'] == foctiles.iloc[i]], prefix, img_folder,
                                                 sample_img_nm, logger)
            bounds = get_extent(tile_geojson, res)
            ard_generation(img_catalog, s3_bucket, prefix, aoi, foctiles.iloc[i], img_folder, proj, bounds, out_folder, logger,
                           dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal)
        logger.info("Finished making planet ARD images: {} for aoi_{}".format(time.time(), aoi))
    # tile-based processing
    else:
        foc_img_catalog = img_catalog.loc[img_catalog['tile'] == int(tile_id)]
        sample_img_nm = foc_img_catalog.iloc[0, 0]
        tile_geojson, proj = get_geojson_pcs(s3_bucket, gjs_tile[gjs_tile['id'] == int(tile_id)], prefix, img_folder, sample_img_nm,
                                             logger)
        bounds = get_extent(tile_geojson, res)
        ard_generation(img_catalog, s3_bucket, prefix, aoi, int(tile_id), img_folder, proj, bounds, out_folder, logger,
                       dry_lower_ordinal, dry_upper_ordinal, wet_lower_ordinal, wet_upper_ordinal)

if __name__ == '__main__':
    main()

