from datetime import datetime, timedelta
from random import choices
from string import ascii_lowercase

from ..create_collection import create_collection
from ..create_item import create_item
from ..create_mosaic_definition import create_mosaic_definition


def test_create_mosaic():
    collection_id_str = "".join(choices(ascii_lowercase, k=15))
    item_id_str = "".join(choices(ascii_lowercase, k=15))
    collection_id = create_collection(
        collection_id_str,
        "test collection",
        [[0, 0, 1, 1]],
        datetime.now() - timedelta(weeks=3),
        datetime.now(),
        "proprietary",
        "franklin.service.internal:9090",
        "http",
    )["id"]
    item_id = create_item(
        item_id_str,
        datetime.now(),
        "/opt/src/test/data/mask-cog.tif",
        collection_id,
        "/opt/src/planet-eo.json",
        0.1,
        "franklin.service.internal:9090",
        "http",
    )["id"]

    items = [{"itemId": item_id, "assetName": "cog"}]

    result = create_mosaic_definition(
        collection_id,
        None,
        (0, 0),
        8,
        items,
        2,
        30,
        [-180, -90, 180, 90],
        "franklin.service.internal:9090",
        "http",
    )

    assert result["items"] == items
