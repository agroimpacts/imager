from datetime import datetime, timedelta
from random import choices
from string import ascii_lowercase

from ..create_collection import create_collection


def test_create_collection():
    collection_id_str = "".join(choices(ascii_lowercase, k=15))
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

    assert collection_id == collection_id_str
