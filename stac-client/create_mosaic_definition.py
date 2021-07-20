import csv
import logging
import sys
from typing import List, Optional, Tuple
from uuid import uuid4

import requests

import click

logger = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
logger.addHandler(handler)
logger.setLevel(logging.INFO)


def create_mosaic_definition(
    collection_id: str,
    description: Optional[str],
    center_xy: Tuple[float, float],
    center_zoom: int,
    items: List[Tuple[str, str]],
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
    except requests.HTTPError as e:
        logger.error(
            f"Something went wrong while creating your collection: {resp.content}"
        )
        raise


def create_mosaic_definition_cmd():
    ...
