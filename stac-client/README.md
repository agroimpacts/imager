`stac-client`
=============

This directory contains software for interacting with a [STAC API server](https://github.com/radiantearth/stac-api-spec) over HTTP.

Its structure mimics the structure of the scripts in the [`planet`](../planet) direectory,
focused on two analogies:

- "projects" in the Raster Foundry vernacular are ["collections"](https://github.com/radiantearth/stac-spec/blob/v1.0.0/collection-spec/collection-spec.md) in STAC.
- "scenes" in the Raster Foundry vernacular are ["items"](https://github.com/radiantearth/stac-spec/blob/v1.0.0/item-spec/item-spec.md) in STAC.

The server implementation we'll use for this example is [Franklin](https://azavea.github.io/franklin), it's easy both to get up and running locally and to deploy it to a cloud environment.