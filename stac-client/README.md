`stac-client`
=============

This directory contains software for interacting with a [STAC API server](https://github.com/radiantearth/stac-api-spec) over HTTP.

Its structure mimics the structure of the scripts in the [`planet`](../planet) direectory,
focused on two analogies:

- "projects" in the Raster Foundry vernacular are ["collections"](https://github.com/radiantearth/stac-spec/blob/v1.0.0/collection-spec/collection-spec.md) in STAC.
- "scenes" in the Raster Foundry vernacular are ["items"](https://github.com/radiantearth/stac-spec/blob/v1.0.0/item-spec/item-spec.md) in STAC.

The server implementation we'll use for this example is [Franklin](https://azavea.github.io/franklin), it's easy both to get up and running locally and to deploy it to a cloud environment.

## Quickstart

The minimum you'll need to get started locally is to install a recent version of `docker` that also bundles `docker compose`.
You can find [installation instructions](https://docs.docker.com/get-docker/) for common operating systems in Docker's documentation.

Once you've installed docker, from this directory, you can run `docker-compose up`. This will create a contrainer running
a PostgreSQL database and another container running Franklin that communicates with that PostgreSQL database.

To verify that everything's running correctly, you can make a request for the landing page. This sample assumes that you have
`curl` installed, but if not you can also use GUI HTTP clients like [Postman](https://www.postman.com/product/api-client/).

```bash
$ curl http://localhost:9090/
```

## Adding data

To see the collections available in your copy of Franklin, you can make a request to the `/collections` endpoint:

```bash
$ curl http://localhost:9090/collections

{"collections":[],"links":[]}
```

Looks like there's nothing there 🤔 but this isn't too unexpected, we just brought up the server. 

### Creating a collection

To make our first collection,
we'll send a POST request with the following data:

```json
{
    "assets": {},
    "description": "My first STAC collection",
    "extent": {
        "spatial": {
            "bbox": [
                [
                    0,
                    0,
                    10,
                    10
                ]
            ]
        },
        "temporal": {
            "interval": [
                [
                    null,
                    null
                ]
            ]
        }
    },
    "id": "my-first-collection",
    "keywords": [],
    "license": "proprietary",
    "links": [],
    "properties": {},
    "providers": [],
    "stac_extensions": [],
    "stac_version": "1.0.0",
    "summaries": {},
    "type": "Collection"
}
```

To send the POST request, you can run the following bash command (I won't go into detail about the `curl` parameters since this is just one way to interact, and the less convenient one),
which sends the JSON above in a POST request to `http://localhost:9090/collections`

```bash
$ echo '{
    "assets": {},
    "description": "My first STAC collection",
    "extent": {
        "spatial": {
            "bbox": [
                [
                    0,
                    0,
                    10,
                    10
                ]
            ]
        },
        "temporal": {
            "interval": [
                [
                    null,
                    null
                ]
            ]
        }
    },
    "id": "my-first-collection",
    "keywords": [],
    "license": "proprietary",
    "links": [],
    "properties": {},
    "providers": [],
    "stac_extensions": [],
    "stac_version": "1.0.0",
    "summaries": {},
    "type": "Collection"
}' | curl -X POST -d@- http://localhost:9090/collections

{"type":"Collection","stac_version":"1.0.0","stac_extensions":[],"id":"my-first-collection","description":"My first STAC collection","keywords":[],"license":"proprietary","providers":[],"extent":{"spatial":{"bbox":[[0.0,0.0,10.0,10.0]]},"temporal":{"interval":[[null,null]]}},"summaries":{},"properties":{},"links":[{"href":"http://localhost:9090/collections/my-first-collection","rel":"self","type":"application/json"},{"href":"http://localhost:9090","rel":"root","type":"application/json"}],"assets":{}}
```

Now if you hit the collections list endpoint, you'll see that the `collections` key of the returned object has your new collection in it.

Collections are a container type -- in STAC APIs, every item lives in a collection.

### Creating an item

Once you have a collection, you can add items to it. An _item_ is an individual piece of GeoJSON metadata. The item holds metadata, and any data that those metadata are valid for live in the item's _assets_. Items look like this:

```json
{
  "id": "public cog",
  "stac_version": "1.0.0",
  "stac_extensions": [],
  "type": "Feature",
  "geometry": {
    "type": "Polygon",
    "coordinates": [
      [
        [
          -115.405456,
          35.545961
        ],
        [
          -115.387092,
          35.545717
        ],
        [
          -115.38602,
          35.599179
        ],
        [
          -115.404396,
          35.599424
        ],
        [
          -115.405456,
          35.545961
        ]
      ]
    ]
  },
  "bbox": [
    -115.405456,
    35.545716999999996,
    -115.38602,
    35.599424
  ],
  "links": [],
  "assets": {
    "TODO need a public COG to put somewhere"
  },
  "collection": "my-first-collection",
  "properties": {
    "datetime": "2014-05-30T16:12:00Z"
  }
}
```