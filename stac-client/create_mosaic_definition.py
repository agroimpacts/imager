import csv
import logging
import sys
from typing import List, Optional, Tuple
from uuid import uuid4


import click
from click.types import ParamType
import requests


from bbox import BBOX_TYPE


logger = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
logger.addHandler(handler)
logger.setLevel(logging.INFO)


class CenterType(ParamType):
    name = "center"

    def convert(self, value, param, ctx):
        if isinstance(value, tuple):
            return value

        try:
            values = value.split(",")
            if len(values) != 2:
                self.fail(
                    f"Need exactly two comma-separated values to unpack for a map center"
                )
            else:
                return (float(values[0]), float(values[1]))
        except ValueError as e:
            self.fail(
                f"Could not read a map center from the string for param {param}. Value: {value}: {e}"
            )


def create_mosaic_definition(
    collection_id: str,
    description: Optional[str],
    center_xy: Tuple[float, float],
    center_zoom: int,
    items: List[dict],
    min_zoom: int,
    max_zoom: int,
    bounds: List[float],
    api_host: str,
    api_scheme: str,
):
    (lon, lat) = center_xy
    mosaic_definition = {
        "id": str(uuid4()),
        "description": description,
        "center": [lon, lat, center_zoom],
        "items": items,
        "minZoom": min_zoom,
        "maxZoom": max_zoom,
        "bounds": bounds,
    }
    try:
        resp = requests.post(
            f"{api_scheme}://{api_host}/collections/{collection_id}/mosaic",
            json=mosaic_definition,
        )
        resp.raise_for_status()
        api_mosaic = resp.json()
        logger.info(
            f"Created mosaic {api_mosaic['id']} at /collections/{collection_id}/mosaic/{api_mosaic['id']}"
        )
        logger.info(
            f"You can view tiles at {api_scheme}://{api_host}/tiles/collections/{collection_id}/mosaic/{api_mosaic['id']}/WebMercatorQuad/{{z}}/{{x}}/{{y}}"
        )
    except requests.HTTPError as e:
        logger.error(
            f"Something went wrong while creating your collection: {resp.content}"
        )
        raise


@click.command()
@click.option(
    "--collection-id",
    help="Which collection holds the items of this mosaic",
    required=True,
)
@click.option(
    "--items-csv",
    type=click.Path(exists=True),
    help="Path to a CSV of (itemId, assetName) pairs",
    required=True,
)
@click.option(
    "-d", "--description", help="Description of the what this mosaic represents"
)
@click.option(
    "--center",
    required=True,
    help="x,y (longitude,latitude) of the center of this mosaic",
    type=CenterType(),
)
@click.option(
    "--center-zoom",
    required=True,
    help="The zoom a client should choose on first load of this mosaic",
)
@click.option(
    "--min-zoom",
    type=int,
    default=2,
    help="Minimum zoom level clients should view to consume this mosaic",
)
@click.option(
    "--max-zoom",
    type=int,
    default=30,
    help="Maximum zoom level clients should view to consume this mosaic",
)
@click.option(
    "--bounds",
    type=BBOX_TYPE,
    default=[-180, -90, 180, 90],
    help="Bounding box in which this mosaic has coverage",
)
@click.option(
    "--api-host",
    help="The root of the STAC API you'd like to interact with. Defaults to localhost:9090",
    default="localhost:9090",
)
@click.option(
    "--api-scheme",
    help="The scheme to use for API communication. Defaults to http",
    default="http",
)
def create_mosaic_definition_cmd(
    collection_id: str,
    items_csv: str,
    description: Optional[str],
    center: Tuple[float, float],
    center_zoom: int,
    min_zoom: int,
    max_zoom: int,
    bounds: List[float],
    api_host: str,
    api_scheme: str,
):
    with open(items_csv, "r") as inf:
        reader = csv.DictReader(inf.readlines(), fieldnames=["itemId", "assetName"])
        items = [x for x in reader]
    return create_mosaic_definition(
        collection_id,
        description,
        center,
        center_zoom,
        items,
        min_zoom,
        max_zoom,
        bounds,
        api_host,
        api_scheme,
    )


if __name__ == "__main__":
    create_mosaic_definition_cmd()
