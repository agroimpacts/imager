# mosaic and COG model predictions for AOIs

import rasterio
import numpy as np
from rasterio.io import MemoryFile
from rasterio.merge import merge
from rasterio.plot import show
import boto3
from rasterio.plot import show
import os
from rio_cogeo.cogeo import cog_translate
from rio_cogeo.profiles import cog_profiles
import re
from subprocess import run

def list_objects(s3_resource, bucket, prefix, suffix=None):
    """
    Get list of keys in an S3 bucket, filtering by prefix and suffix. Function
    developed by Kaixi Zhang as part of AWS_S3 class and adapted slightly here.
    This function retrieves all matching objects, and is not subject to the 1000
    item limit.
        Params:
            s3_resource (object): A boto3 s3 resource object
            bucket (str): Name of s3 bucket to list
            prefix (str): Prefix within bucket to search
            suffix (str, list): Optional string or string list of file endings
        Returns:
            List of s3 keys
    """
    keys = []
    if s3_resource is not None:
        s3_bucket = s3_resource.Bucket(bucket)
        for obj in s3_bucket.objects.filter(Prefix=prefix):
            # if no suffix given, add all objects with the prefix
            if suffix is None:
                keys.append(str(obj.key))
            else:
                # add all objects that ends with the given suffix
                if isinstance(suffix, list):
                    for _suffix in suffix:
                        if obj.key.endswith(_suffix):
                            keys.append(str(obj.key))
                            break
                else:
                    # suffix is a single string
                    if obj.key.endswith(suffix):
                        keys.append(str(obj.key))
    else:
        print
        'Warning: please first create an s3 resource'
    return keys

# Mosaick local
def mosaic(images, fileout):

    # merge
    print('Merging images')
    array, out_trans = merge(images) 

    # profile 
    profile = images[0].profile
    profile['transform'] = out_trans
    profile['height'] = array.shape[1]
    profile['width'] = array.shape[2]
    profile['driver'] = 'GTiff'
    profile['compress'] = "lzw"
    # profile['tiled'] = True
    # profile['dtype'] = 'int8'
    
    print('Writing to disk')
    # write to local
    
    try:
        with rasterio.open(fileout, 'w', **profile) as dst:
            dst.write(array)
            
    except:
        print('Write failure for {}'.format(fileout))
        
        
bucket = '***REMOVED***'
prefix_base = 'DL/Result/semantic_segmentation/boka/relabeller/coarse_as_benchmark/' + \
    'balanced_tversky_g09/new/d3500/refine_freeze58/ep150/predict/'

# local directory
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(os.getcwd())))
local_dir = os.path.join(ROOT, 'imager/planet/catalog/')

# resource/client
s3 = boto3.resource('s3')
s3_client = boto3.client('s3') # client, for later use

# processing in loop 
for i in range(13, 14): 
    # set up file paths
    input_prefix = '{}predict_{}/Score_1/'.format(prefix_base, i)
    output_prefix = 'maps/ghana/score_1/'  
    output_image = 'ghana_score1_{}_v0_1.tif'.format(i)
    
    # output files. Only COG goes to S3
    local_file = '{}{}'.format(local_dir, output_image)
    cog_name = re.sub('.tif', '_cog.tif', output_image)
    local_cog_file = '{}{}'.format(local_dir, cog_name)
    output_key = '{}{}'.format(output_prefix, cog_name) # COG to S3

    # image keys
    print("Getting keys {}".format(input_prefix))
    keys = list_objects(s3, bucket, input_prefix, 'tif')
    keys_full = ['s3://***REMOVED***/%s' % (key) for key in keys]
    
    # Read in images
    print("Reading images from {}".format(input_prefix))
    images = [rasterio.open(key) for key in keys_full]
    
    # create local mosaic
    print("Creating mosaic {}".format(local_file))
    mosaic(images, local_file)
    
    # create COG
    print("Creating cog from {}".format(local_file))
    cmd = ['rio', 'cogeo', 'create', '--nodata', '-128', local_file, local_cog_file]
    p = run(cmd, capture_output=True)
    print(p.stderr.decode())
    
    # validate COG
    print("Validating {}".format(local_cog_file))
    cmd = ['rio', 'cogeo', 'validate', local_cog_file]
    p = run(cmd, capture_output=True)
    print(p.stdout.decode())
    
    # upload to S3
    print("Uploading {} to S3".format(cog_name))
    s3_client.upload_file(local_cog_file, bucket, output_key)
    
    print("")