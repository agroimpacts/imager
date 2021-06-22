import yaml
import argparse
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)
rfclient.datasource = {"id": "c841a14a-87ae-474e-ac5b-8b4fd41cb5b5"}
rfclient.bands = [{
                    "number": 0,
                    "name": "Band1",
                    "wavelength": [11907, 17118]
                  },
                  {
                    "number": 1,
                    "name": "Band2",
                    "wavelength": [5683, 11015]
                  },
                  {
                    "number": 2,
                    "name": "Band3",
                    "wavelength": [0, 4]
                  },
                  {
                    "number": 3,
                    "name": "Band4",
                    "wavelength": [0, 5]
                  }

]


def main(scene_id, url):
    scene = rfclient.create_scene(scene_id, url)
    print(scene.id)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate new TMS links for each image')
    parser.add_argument('--scene_id', help='The scene id of the image')
    parser.add_argument('--url', help='The url of the image')
    args = parser.parse_args()
    main(args.scene_id, args.url)
