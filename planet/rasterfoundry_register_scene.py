import yaml
import argparse
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)


def main(scene_id, url):
    scene = rfclient.create_scene(scene_id, url)
    print(scene.id)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = 'Generate new TMS links for each image')
    parser.add_argument('--scene_id', help='The scene id of the image')
    parser.add_argument('--url', help='The url of the image')
    args = parser.parse_args()
    main(args.scene_id, args.url)
