import yaml
import argparse
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)


def main(scene_id, scene_ids):
        tms_uri = ''
        scene_ids = scene_ids.split(",")
        if rfclient.enabled:
            try:
                new_project = rfclient.create_project("Project {}".format(scene_id),
                                                      rfclient.visibility, rfclient.tileVisibility)
                result = rfclient.add_scenes_to_project_id(scene_ids, new_project)
                tms_uri = rfclient.tms_with_map_token(new_project)
            except:
                rfclient.logger.exception('Error Encountered')
                rfclient.logger.info("An error happened during RF TMS URI creation, scene_id: {}, scene_uri: {}".format(scene_id, scene_uri))

        print(tms_uri)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = 'Generate new TMS links for each image')
    parser.add_argument('--scene_id', help='The scene id of the image')
    parser.add_argument('--scene_ids', help='The ids of the NB')
    args = parser.parse_args()
    main(args.scene_id, args.scene_ids)
