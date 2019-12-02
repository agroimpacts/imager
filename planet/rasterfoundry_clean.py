# author name: Lei Song
# description: functions to clean the raster Foundary directory.

import yaml
import requests
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)


def list_owned_scenes(user_id, jwt):
    """
    List all owned scenes
    Args:
        user_id: rfclient.owner
        jwt: rfclient.api.api_token
    Returns:
        list of scenes
    """
    def make_request(headers, page, page_size=1000):
        return requests.get(
            'https://{host}/api/scenes'.format(host=rf_host),
            headers=headers,
            params={
                'pageSize': page_size,
                'page': page,
                'owner': user_id
            }
        )
    headers = {'Authorization': jwt}
    rf_host = 'app.rasterfoundry.com'
    page = 0
    resp = make_request(headers, page)
    resp.raise_for_status()
    js = resp.json()
    scenes = js['results']
    has_next = js['hasNext']

    while has_next:
        page += 1
        resp = make_request(headers, page)
        resp.raise_for_status()
        js = resp.json()
        scenes.extend(js['results'])
        has_next = js['hasNext']

    return scenes


def main():
    rfclient.delete_all_projects()
    scenes = list_owned_scenes(user_id=rfclient.owner, jwt=rfclient.api.api_token)
    for scene in scenes:
        rfclient.api.client.Imagery.delete_scenes_sceneID(sceneID=scene['id']).result()


if __name__ == "__main__":
    main()
