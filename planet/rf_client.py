from rasterfoundry.api import API
from rasterfoundry.models import Project
from rasterfoundry.models import MapToken

import logging
import configparser
import ssl
import jwt
import uuid
import json

# https://docs.rasterfoundry.com/#/
class RFClient():
    def __init__(self, config):
        rf_config = config['rasterfoundry']
        imagery_config = config['imagery']
        self.api_key = rf_config['api_key']
        self.api_uri = rf_config['api_uri']
        self.visibility = rf_config['visibility']
        self.tileVisibility = rf_config['tileVisibility']
        # it's enabled only in s3 mode and with explicit enabled flag
        self.enabled = rf_config['enabled']
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.INFO)
        if self.enabled:
            self.api = API(refresh_token = self.api_key, host = self.api_uri)
            self.api_token_decoded = jwt.decode(self.api.api_token, algorithms = ['RS256'], verify = False)
            self.owner = self.api_token_decoded['sub']
        else:
            self.api = None
            self.api_token_decoded = None
            self.owner = None
        self.datasource = {"id": "e4d1b0a0-99ee-493d-8548-53df8e20d2aa"} # 4-band PlanetScope
        # https://assets.planet.com/docs/Planet_Combined_Imagery_Product_Specs_letter_screen.pdf
        self.bands = [
                {
                    "number": 0,
                    "name": "Blue",
                    "wavelength": [455, 515]
                },
                {
                    "number": 1,
                    "name": "Green",
                    "wavelength": [500, 590]
                },
                {
                    "number": 2,
                    "name": "Red",
                    "wavelength": [590, 670]
                },
                {
                    "number": 3,
                    "name": "Near Infrared",
                    "wavelength": [780, 860]
                }
        ]

    def refresh(self):
        if self.enabled:
            self.api = API(refresh_token = self.api_key, host = self.api_uri)
            self.api_token_decoded = jwt.decode(self.api.api_token, algorithms = ['RS256'], verify = False)
            self.owner = self.api_token_decoded['sub']

    def tms_with_map_token(self, project):
        url = ''
        try:
            tile_path = '/tiles/{id}/{{z}}/{{x}}/{{y}}/'.format(id = project.id)
            token = self.create_map_token(project).id

            url = '{scheme}://{host}{tile_path}?mapToken={token}'.format(
                scheme=self.api.scheme, host=self.api.tile_host,
                tile_path=tile_path, token=token
            )
        except:
            self.logger.exception('Error Encountered')
            self.logger.info("Oops, could not get mapToken, using a common uri with token...")
            url = project.tms()
        
        return url

    def create_project(self, project_name, visibility = 'PRIVATE', tileVisibility = 'PRIVATE'):
        return self.api.client.Imagery.post_projects(
            project = {
                "name": project_name,
                "description": "mapperAL generated project for TMS view",
                "visibility": visibility,
                "tileVisibility": tileVisibility,
                "owner": self.owner,
                "tags": ["Planet Scene mapperAL project"]
            }
        ).result()

    def create_scene(self, scene_name, uri, visibility = 'PRIVATE'):
        scene_uuid = str(uuid.uuid4())
        return self.api.client.Imagery.post_scenes(
            scene = {
                "id": scene_uuid,
                "owner": self.owner,
                "name": scene_name,
                "ingestLocation": uri,
                "visibility": visibility,
                "images": [{
                    "rawDataBytes": 0,
                    "visibility": visibility,
                    "filename": uri,
                    "sourceUri": uri,
                    "scene": scene_uuid,
                    "imageMetadata": {},
                    "resolutionMeters": 0,
                    "metadataFiles": [],
                    "bands": self.bands
                }],
                "thumbnails": [],
                "sceneMetadata": {},
                "metadataFiles": [],
                "sceneType": "COG",
                "filterFields": {},
                "tags": [
                    "Planet Scenes",
                    "GeoTIFF"
                ],
                "statusFields": {
                    "thumbnailStatus": "SUCCESS",
                    "boundaryStatus": "SUCCESS",
                    "ingestStatus": "INGESTED"
                },
                "datasource": self.datasource
            }
        ).result()

    def add_scenes_to_project(self, scenes, project):
        return self.api.client.Imagery.post_projects_projectID_scenes(
            projectID = project.id, 
            scenes = [scene.id for scene in scenes]
        ).future.result()

    def add_scenes_to_project_id(self, scenes, project):
        return self.api.client.Imagery.post_projects_projectID_scenes(
            projectID = project.id,
            scenes = scenes
        ).future.result()

    def create_map_token(self, project):
        return self.api.client.Imagery.post_map_tokens(MapToken = {
            'name': 'planet_downloader generated token', 
            'project': project.id
        }).result()

    def delete_project(self, project):
        return self.api.client.Imagery.delete_projects_projectID(projectID = project.id).result()
        
    def delete_scene(self, scene):
        return self.api.client.Imagery.delete_scenes_sceneID(sceneID=scene.id)

    def delete_all_projects(self):
        for project in self.api.projects:
            try:
                self.delete_project(project)
                self.logger.info("Project {} deleted".format(project.id))
            except:
                self.logger.exception('Error Encountered')
                self.logger.info("Project {} can't be deleted".format(project.id))

    def delete_all_scenes(self):
        for scene in self.api.api.get_scenes().results:
            try:
                self.delete_scene(scene)
                self.logger.info("Scene {} deleted".format(scene.id))
            except:
                self.logger.exception('Error Encountered')
                self.logger.info("Scene {} can't be deleted".format(scene.id))

    def create_scene_project(self, scene_id, scene_uri):
        if self.enabled:
            new_project = self.create_project("Project {}".format(scene_id), self.visibility, self.tileVisibility)
            new_scene = self.create_scene(scene_id, scene_uri, self.visibility)
            result = self.add_scenes_to_project([new_scene], new_project)
            return new_scene, Project(new_project, self.api)

    def create_scenes_project(self, scene_id, scene_uri):
        if self.enabled:
            new_project = self.create_project("Project {}".format(scene_id), self.visibility, self.tileVisibility)
            new_scene = self.create_scene(scene_id, scene_uri, self.visibility)
            result = self.add_scenes_to_project([new_scene], new_project)
            return new_scene, Project(new_project, self.api)

    def create_tms_uri(self, scene_id, scene_uri):
        tms_uri = ''
        if self.enabled:
            try:
                new_scene, new_project = self.create_scene_project(scene_id, scene_uri)
                tms_uri = self.tms_with_map_token(new_project)
            except:
                self.logger.exception('Error Encountered')
                self.logger.info("An error happened during RF TMS URI creation, scene_id: {}, scene_uri: {}".format(scene_id, scene_uri))

        return tms_uri

# example main function        
def main():
    # disable ssl
    # ssl._create_default_https_context = ssl._create_unverified_context

    # logging format
    # logging.basicConfig(format = '%(message)s', datefmt = '%m-%d %H:%M')

    # read config
    config = configparser.ConfigParser()
    config.read('cfg/config.ini')

    rfclient = RFClient(config)

    # delete all projects
    # rfclient.delete_all_projects()
    
    scene_uri = "s3://***REMOVED***/planet/composite_sr/GS/tile487089_736815_736967.tif"
    scene_id = "tile487089_736815_736967"

    tms_uri = rfclient.create_tms_uri(scene_id, scene_uri)

    print(tms_uri)

if __name__ == "__main__":
    main()
