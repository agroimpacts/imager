"""
This is quick and simple tool to extract all scenes registered in Raster foundry.
and save out a csv file. It is not a perfect script, so use it with your own adaptation.
Author: Lei Song
"""
import yaml
import pandas as pd
from rasterfoundry_clean import list_owned_scenes
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)

scenes = list_owned_scenes(user_id=rfclient.owner, jwt=rfclient.api.api_token)
scenes_pd = pd.DataFrame(columns=['name', 'scene_id', 'ingestLocation', 'time'])

for scene in scenes:
    scenes_pd = scenes_pd.append({'name': scene['name'], 'scene_id': scene['id'],
                                  'ingestLocation': scene['ingestLocation'],
                                  'time': scene['createdAt']},
                                 ignore_index=True)
scenes_pd.to_csv("planet/data/scenes_all.csv", index=False)
