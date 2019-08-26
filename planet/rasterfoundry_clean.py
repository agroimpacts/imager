import yaml
import argparse
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)

# rfclient init
rfclient = RFClient(config)


def main():
    rfclient.delete_all_projects()
    rfclient.delete_all_scenes()


if __name__ == "__main__":
    main()
