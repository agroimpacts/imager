import yaml
import argparse
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)
rfclient.datasource = {"id": "6d03b24d-5a29-4004-a27f-3dda48e2eedb"}
rfclient.bands = [{
                    "number": 0,
                    "name": "Red",
                    "wavelength": [0, 100]
                  },
                  {
                    "number": 1,
                    "name": "Green",
                    "wavelength": [0, 100]
                  },
                  {
                    "number": 2,
                    "name": "Blue",
                    "wavelength": [0, 100]
                  }

]


def main(scene_id, url):
    scene = rfclient.create_scene(scene_id, url)
    print(scene.id)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = 'Generate new TMS links for each image')
    parser.add_argument('--scene_id', help='The scene id of the image')
    parser.add_argument('--url', help='The url of the image')
    args = parser.parse_args()
    main(args.scene_id, args.url)
