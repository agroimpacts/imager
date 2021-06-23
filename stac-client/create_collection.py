from datetime import datetime
import logging
import sys
from typing import List, Optional

import click
from click.types import ParamType
from pystac import Collection
from pystac.catalog import CatalogType
from pystac.collection import Extent, SpatialExtent, TemporalExtent
import requests


logger = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
logger.addHandler(handler)


class BboxType(ParamType):

    name = "bbox"

    def convert(self, value, param, ctx):
        # we're in the default case, so just return it
        if value == [0, 0, 0, 0]:
            return value

        try:
            num_strings = value.split(",")
            converted = [float(x) for x in num_strings]
            if len(converted) == 4:
                return converted
            else:
                self.fail(
                    f"Needed exactly four values for the bounding box. Got {len(converted)}."
                )
        except ValueError as e:
            self.fail(
                f"Could not read a bbox rom the string for param {param}. Value: {value}: {e}"
            )


BBOX_TYPE = BboxType()

time_format_hint = "Provide as an ISO 8601 timestamp without a time zone -- it will be parsed as UTC. Defaults to now."


@click.command()
@click.argument("name")
@click.option(
    "-d",
    "--description",
    help="Collection description. This should be a short punchy phrase that describes what kinds of items can be found in this collection",
    default="A STAC collection for Agro Impacts",
)
@click.option(
    "--bbox",
    help="Bounding box in comma-separated lower left x, lower left y, upper right x, upper right y format. This will expand as you add items, so picking a small bbox in the appropriate area makes sense to start.",
    type=BBOX_TYPE,
    default=[0, 0, 0, 0],
)
@click.option(
    "--start-date",
    help=f"Datetime marking the beginning of coverage for this collection. {time_format_hint}",
    type=click.DateTime(),
    default=None,
)
@click.option(
    "--end-date",
    help=f"Datetime marking the end of coverage for this collection. {time_format_hint}",
    type=click.DateTime(),
    default=None,
)
@click.option(
    "--license",
    help='License of the underlying data. You can use identifiers from https://spdx.github.io/license-list-data/ in this field or the string "proprietary"',
    default="proprietary",
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
def create_collection(
    name: str,
    description: str,
    bbox: List[float],
    start_date: Optional[datetime],
    end_date: Optional[datetime],
    license: str,
    api_host: str,
    api_scheme: str,
):
    # TODO: root link is getting a null href -- should probably just remove all links before creating it as
    # a first workaround
    collection = Collection(
        name,
        description,
        Extent(SpatialExtent(bbox), TemporalExtent([start_date, end_date])),
        title=name,
        stac_extensions=None,
        href=None,
        extra_fields=None,
        catalog_type=CatalogType.SELF_CONTAINED,
        license=license,
        summaries=[],
    )

    # we have to empty out the links, because the collection by default gets a root link with a null href
    collection.links = []

    try:
        resp = requests.post(
            f"{api_scheme}://{api_host}/collections", json=collection.to_dict()
        )
        resp.raise_for_status()
        return resp.json()
    except requests.HTTPError as e:
        logger.error(
            f"Something went wrong while creating your collection: {resp.content}"
        )
        raise


if __name__ == "__main__":
    create_collection()
