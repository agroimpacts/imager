import datetime as dt
import json
import logging
import sys
from typing import Optional

import click
import rasterio
from click.types import ParamType
from pystac import Asset, MediaType
from pystac.errors import ExtensionNotImplemented
from pystac.extensions.eo import ItemEOExtension
from pystac.item import Item
from rasterio import RasterioIOError
from rasterio.crs import CRS
from rasterio.vrt import WarpedVRT
import requests
from requests.exceptions import HTTPError
from rio_cogeo.cogeo import cog_validate
from shapely.geometry import Polygon, mapping

logger = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
logger.addHandler(handler)
logger.setLevel(logging.INFO)

eo_help = (
    "Path to JSON containing information to fill in the EO extension for this "
    "item. You can read more about the EO extension at "
    "https://github.com/stac-extensions/eo/tree/v1.0.0"
)


class COGPathType(ParamType):

    name = "cog-path"

    def convert(self, value, param, ctx):
        if isinstance(value, COGPathType):
            return value

        try:
            (valid, errs, _) = cog_validate(value, strict=True)
            if not valid:
                self.fail(
                    f"The data at {value} are not a valid COG. Data was there, but it was not a COG: {errs}"
                )
            else:
                return value
        except RasterioIOError as e:
            self.fail(f"Could not read a COG at {value}: {e}")


def create_item(
    item_id,
    datetime,
    cog_path,
    collection_id,
    eo_data,
    cloud_cover,
    api_host,
    api_scheme,
):
    data_asset = Asset(cog_path, media_type=MediaType.COG, roles=["data"])
    if eo_data is not None:
        with open(eo_data, "r") as inf:
            eo_bands = json.load(inf)
    else:
        eo_bands = {}
    with rasterio.open(cog_path, "r") as src:
        vrt = WarpedVRT(src, crs=CRS.from_epsg(4326))
        lat_long_bounds = vrt.bounds
        bbox = list(lat_long_bounds)
        lat_long_geom = Polygon.from_bounds(*bbox)
        lat_long_geom_geojson = mapping(lat_long_geom)

    item = Item(
        item_id,
        lat_long_geom_geojson,
        bbox,
        datetime,
        properties=eo_bands | {"eo:cloud_cover": cloud_cover},
    )

    try:
        resp = requests.post(
            f"{api_scheme}://{api_host}/collections/{collection_id.replace(' ', '+')}/items",
            json=item.to_dict(),
        )
        resp.raise_for_status()
        api_item = resp.json()
        item_parent_link = [
            x["href"] for x in api_item["links"] if x["rel"].lower() == "parent"
        ][0]
        logger.info(f"Created item at {item_parent_link}/items/{api_item['id']}")
        return api_item
    except HTTPError as e:
        logger.error(
            f"Something went wrong while creating your collection: {resp.content}"
        )
        raise


@click.command()
@click.argument("item-id", type=str)
@click.argument("datetime", type=click.DateTime())
@click.argument("cog-path", type=COGPathType())
@click.argument("collection-id", type=str)
@click.option("--eo-data", type=click.Path(exists=True), help=eo_help)
@click.option("--cloud-cover", type=float, help="Estimated cloud cover of the image")
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
def create_item_cmd(
    item_id: str,
    datetime: dt.datetime,
    cog_path: str,
    collection_id: str,
    eo_data: str,
    cloud_cover: Optional[float],
    api_host: str,
    api_scheme: str,
):
    return create_item(
        item_id,
        datetime,
        cog_path,
        collection_id,
        eo_data,
        cloud_cover,
        api_host,
        api_scheme,
    )


if __name__ == "__main__":
    create_item_cmd()
