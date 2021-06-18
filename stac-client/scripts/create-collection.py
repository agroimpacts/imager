from datetime import datetime

import click
from pystac import Collection
from pystac.catalog import CatalogType
from pystac.collection import Extent, SpatialExtent, TemporalExtent


@click.argument("name")
@click.option(
    "-d",
    "--description",
    "Collection description. This should be a short punchy phrase that describes what kinds of items can be found in this collection",
)
def create_collection(
    name,
    description="A STAC collection for Clark U",
    bbox=[0, 0, 0, 0],
    start_date=datetime.now(),
    end_date=datetime.now(),
    license="proprietary",
):
    return Collection(
        name,
        description,
        Extent(SpatialExtent(bbox), TemporalExtent([start_date, end_date])),
        title=name,
        stac_extensions=None,
        href=None,
        extra_fields=None,
        catalog_type=CatalogType.SELF_CONTAINED,
        license=license,
    )
