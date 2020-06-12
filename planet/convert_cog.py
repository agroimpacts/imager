"""
This module is to batch convert single band probability maps in S3 folder to
a RGB color-relief probability maps to another S3 folder before register them
to raster foundry.
Author: Lei Song
"""
import os
import yaml
import boto3
import tempfile
import subprocess
from subprocess import check_call
from matplotlib.cm import get_cmap
from os.path import join as pjoin, basename


"""
S3 related code: https://alexwlchan.net/2019/07/listing-s3-keys/
"""


def get_matching_s3_objects(bucket, prefix="", suffix=""):
    """
    Generate objects in an S3 bucket.

    :param bucket: Name of the S3 bucket.
    :param prefix: Only fetch objects whose key starts with
        this prefix (optional).
    :param suffix: Only fetch objects whose keys end with
        this suffix (optional).
    """
    s3 = boto3.client("s3")
    paginator = s3.get_paginator("list_objects_v2")

    kwargs = {'Bucket': bucket}

    # We can pass the prefix directly to the S3 API.  If the user has passed
    # a tuple or list of prefixes, we go through them one by one.
    if isinstance(prefix, str):
        prefixes = (prefix, )
    else:
        prefixes = prefix

    for key_prefix in prefixes:
        kwargs["Prefix"] = key_prefix

        for page in paginator.paginate(**kwargs):
            try:
                contents = page["Contents"]
            except KeyError:
                break

            for obj in contents:
                key = obj["Key"]
                if key.endswith(suffix):
                    yield obj


def get_matching_s3_keys(bucket, prefix="", suffix=""):
    """
    Generate the keys in an S3 bucket.

    :param bucket: Name of the S3 bucket.
    :param prefix: Only fetch keys that start with this prefix (optional).
    :param suffix: Only fetch keys that end with this suffix (optional).
    """
    for obj in get_matching_s3_objects(bucket, prefix, suffix):
        yield obj["Key"]


"""
COG related code is adapted from
https://github.com/harshurampur/Geotiff-conversion
"""


def run_command(command, work_dir):
    """
     A simple utility to execute a subprocess command.
    """
    try:
        check_call(command, stderr=subprocess.STDOUT, cwd=work_dir)
    except subprocess.CalledProcessError as e:
        raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))


def convert_cogtiff(fname, bucket, s3_perfix):
    """ Convert the Geotiff to COG and upload to S3 bucket using gdal commands
        Blocksize is 512
        TILED <boolean>: Switch to tiled format
        COPY_SRC_OVERVIEWS <boolean>: Force copy of overviews of source dataset
        COMPRESS=[NONE/DEFLATE]: Set the compression to use. DEFLATE is only available if NetCDF has been compiled with
                  NetCDF-4 support. NC4C format is the default if DEFLATE compression is used.
        ZLEVEL=[1-9]: Set the level of compression when using DEFLATE compression. A value of 9 is best,
                      and 1 is least compression. The default is 1, which offers the best time/compression ratio.
        BLOCKXSIZE <int>: Tile Width
        BLOCKYSIZE <int>: Tile/Strip Height
        PREDICTOR <int>: Predictor Type (1=default, 2=horizontal differencing, 3=floating point prediction)
        PROFILE <string-select>: possible values: GDALGeoTIFF,GeoTIFF,BASELINE,
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        temp_fname = pjoin(tmpdir, basename(fname))
        temp_out = pjoin(tmpdir, "temp_out.tif")

        env = ['GDAL_DISABLE_READDIR_ON_OPEN=YES',
               'CPL_VSIL_CURL_ALLOWED_EXTENSIONS=.tif']
        subprocess.check_call(env, shell=True)
        
        # Conver to RGB
        color_file = gen_color_file()
        to_cogtif = [
            'gdaldem',
            'color-relief',
            fname,
            color_file,
            temp_fname]
        run_command(to_cogtif, tmpdir)

        # Add Overviews
        # gdaladdo - Builds or rebuilds overview images.
        # 2, 4, 8,16,32 are levels which is a list of integral overview levels to build.
        add_ovr = [
            'gdaladdo',
            '-r',
            'average',
            '--config',
            'GDAL_TIFF_OVR_BLOCKSIZE',
            '512',
            temp_fname,
            '2',
            '4',
            '8',
            '16',
            '32']
        run_command(add_ovr, tmpdir)

        # Convert to COG
        cogtif = [
            'gdal_translate',
            '-co',
            'TILED=YES',
            '-co',
            'COPY_SRC_OVERVIEWS=YES',
            '-co',
            'COMPRESS=DEFLATE',
            '-co',
            'ZLEVEL=9',
            '--config',
            'GDAL_TIFF_OVR_BLOCKSIZE',
            '512',
            '-co',
            'BLOCKXSIZE=512',
            '-co',
            'BLOCKYSIZE=512',
            '-co',
            'PREDICTOR=1',
            '-co',
            'PROFILE=GeoTIFF',
            temp_fname,
            temp_out]
        run_command(cogtif, tmpdir)

        # Upload S3
        s3 = boto3.client('s3')
        s3_path = pjoin(s3_perfix, basename(fname))
        s3.upload_file(temp_out,
                       bucket,
                       s3_path)


def gen_color_file():
    """
     A function to generate color file for gdaldem.
     The rows of file should be [value R G B]
    """
    fp, temp_file = tempfile.mkstemp(suffix='.txt')
    max_ph = 100
    min_ph = 0
    range_ph = max_ph-min_ph
    # Manually define a viridis palette
    # which could be nicer
    rs = ['68', '72', '72', '69', '63', '57', '50', '45',
          '40', '35', '31', '32', '41', '60', '86', '116',
          '148', '184', '220', '253']
    gs = ['1', '21', '38', '55', '71', '85', '100', '113',
          '125', '138', '150', '163', '175', '188', '198',
          '208', '216', '222', '227', '231']
    bs = ['84', '104', '119', '129', '136', '140', '142',
          '142', '142', '141', '139', '134', '127', '117',
          '103', '85', '64', '41', '24', '37']
    with open(temp_file, 'w') as f:
        for i, c in enumerate(rs[:-1]):
            f.write(str(int(min_ph + (i + 1) * range_ph / len(rs))) +
                    ' ' + c + ' ' + gs[i] + ' ' + bs[i] + '\n')
        f.write(str(int(max_ph - range_ph / len(rs))) +
                ' ' + rs[-1] + ' ' + gs[-1] + ' ' + bs[-1] + '\n')
    os.close(fp)
    return temp_file


"""
Main function
"""


def main(config_name):
    # set up environment
    # config_name = "cfg/config.yaml"
    with open(config_name, 'r') as yaml_file:
        params = yaml.load(yaml_file)

    # get Geotiffs within S3 bucket
    tiffs = get_matching_s3_keys(bucket=params['probability']['s3_bucket'],
                                 prefix=params['probability']['s3_tiff_perfix'])

    # convert geotiffs to COGtiffs
    for tiff in tiffs:
        dir_tif = pjoin(params['probability']['s3_header'],
                        params['probability']['s3_bucket'], tiff)
        convert_cogtiff(dir_tif,
                        params['probability']['s3_bucket'],
                        params['probability']['s3_cogtiff_perfix'])


if __name__ == "__main__":
    main("cfg/config.yaml")
