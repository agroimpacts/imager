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

## Adding data the hard way

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
          -15.0002052,
          28.9282827
        ],
        [
          -15.0002033,
          27.9371483
        ],
        [
          -13.8842036,
          27.9326277
        ],
        [
          -13.8737609,
          28.9235723
        ],
        [
          -15.0002052,
          28.9282827
        ]
      ]
    ]
  },
  "bbox": [
    -15.002052,
    27.9326277,
    -13.8737609,
    28.9282827
  ],
  "links": [],
  "assets": {
    "data": {
      "href": "s3://rasterfoundry-production-data-us-east-1/demo-cogs/s2-canary-islands-rgb-cog.tif",
      "title": "RGB COG from Sentinel-2 bands",
      "description": "Composite image from bands 4, 3, and 2 of a Sentinel-2 image over the Canary Islands",
      "roles": ["data"],
      "type": "image/tiff; application=geotiff; profile=cloud-optimized" 
    }
  },
  "collection": "my-first-collection",
  "properties": {
    "datetime": "2021-06-16T00:00:00Z"
  }
}
```

In this case, `assets.data` refers to a COG that lives on S3. You can POST that item to the collection you
created a moment ago like so:

```bash
$ echo '{
  "id": "public cog",
  "stac_version": "1.0.0",
  "stac_extensions": [],
  "type": "Feature",
  "geometry": {
    "type": "Polygon",
    "coordinates": [
      [
        [
          -15.0002052,
          28.9282827
        ],
        [
          -15.0002033,
          27.9371483
        ],
        [
          -13.8842036,
          27.9326277
        ],
        [
          -13.8737609,
          28.9235723
        ],
        [
          -15.0002052,
          28.9282827
        ]
      ]
    ]
  },
  "bbox": [
    -15.002052,
    27.9326277,
    -13.8737609,
    28.9282827
  ],
  "links": [],
  "assets": {
    "data": {
      "href": "s3://rasterfoundry-production-data-us-east-1/demo-cogs/s2-canary-islands-rgb-cog.tif",
      "title": "RGB COG from Sentinel-2 bands",
      "description": "Composite image from bands 4, 3, and 2 of a Sentinel-2 image over the Canary Islands",
      "roles": ["data"],
      "type": "image/tiff; application=geotiff; profile=cloud-optimized" 
    }
  },
  "collection": "my-first-collection",
  "properties": {
    "datetime": "2021-06-16T00:00:00Z"
  }
}' | curl -X POST -d@- http://localhost:9090/collections/my-first-collection/items
```

Then if you list your collections items:

```bash
$ curl http://localhost:9090/collections/my-first-collection/items
```

You'll see the item you created.

Most STAC interaction is pretty much like this -- it centers on items and collections, and sometimes
some extra metadata that you can attach to collections and items. Scripts in this directory will make that interaction
easier and do it with Python instead of bash commands.

## Adding data with scripts

### Creating a collection with `create_collection.py`

Some of the information above will be pretty consistent, or at least we can choose sensible defaults. For example, the
license on collections could default to proprietary, optional fields could default to `null`, etc.
Because of that, we should be able to create collections without having to supply the whole JSON document. That's what the
`create_collection.py` script is for:

```bash
$ python create_collection.py --help
Usage: create_collection.py [OPTIONS] NAME

Options:
  -d, --description TEXT          Collection description. This should be a
                                  short punchy phrase that describes what
                                  kinds of items can be found in this
                                  collection
  --bbox BBOX                     Bounding box in comma-separated lower left
                                  x, lower left y, upper right x, upper right
                                  y format. This will expand as you add items,
                                  so picking a small bbox in the appropriate
                                  area makes sense to start.
  --start-date [%Y-%m-%d|%Y-%m-%dT%H:%M:%S|%Y-%m-%d %H:%M:%S]
                                  Datetime marking the beginning of coverage
                                  for this collection. Provide as an ISO 8601
                                  timestamp without a time zone -- it will be
                                  parsed as UTC. Defaults to now.
  --end-date [%Y-%m-%d|%Y-%m-%dT%H:%M:%S|%Y-%m-%d %H:%M:%S]
                                  Datetime marking the end of coverage for
                                  this collection. Provide as an ISO 8601
                                  timestamp without a time zone -- it will be
                                  parsed as UTC. Defaults to now.
  --license TEXT                  License of the underlying data. You can use
                                  identifiers from
                                  https://spdx.github.io/license-list-data/ in
                                  this field or the string "proprietary"
  --api-host TEXT                 The root of the STAC API you'd like to
                                  interact with. Defaults to localhost:9090
  --api-scheme TEXT               The scheme to use for API communication.
                                  Defaults to http
  --help                          Show this message and exit.
```

Generally speaking, it's valuable to specify at least the beginning `--start-date` and `--end-date`
and a `--bbox` in the appropriate area where you expect to add data. These fields are valuable
for getting a quick summary of where and when items in this collection are located. If you're not
pointing to a local Franklin instance, you can override where the server lives with the `--api-host`
and `--api-scheme` arguments. And if you want to override the `--license` or `--description`, you can
provide strings for those as well.

For a small example, you could create a collection like this, if you have the local server and database
running:

```bash
$ python create_collection.py --start-date '2021-01-01' --end-date '2021-05-01' --bbox 0,0,1,1 'my-great-collection'
```

### Creating an item with `create_item.py`

Items are a little bit more restrictive -- more of their fields are required. For `create_item.py`, we also add
a restriction that we _must_ have a COG asset available. Help for `create_item.py` looks like this:

```bash
$ python create_item.py --help
Usage: create_item.py [OPTIONS] ITEM_ID [%Y-%m-%d|%Y-%m-%dT%H:%M:%S|%Y-%m-%d
                      %H:%M:%S] COG_PATH COLLECTION_ID

Options:
  --eo-data PATH       Path to JSON containing information to fill in the EO
                       extension for this item. You can read more about the EO
                       extension at https://github.com/stac-
                       extensions/eo/tree/v1.0.0
  --cloud-cover FLOAT  Estimated cloud cover of the image
  --api-host TEXT      The root of the STAC API you'd like to interact with.
                       Defaults to localhost:9090
  --api-scheme TEXT    The scheme to use for API communication. Defaults to
                       http
  --help               Show this message and exit.
```

For creating an item, more fields are required:

- `ITEM_ID` is the id of the item. This must be unique across all items you create, so choose IDs that convey enough information that you won't have collisions.
- The next required argument is the timestamp --  you can see that there are several accepted formats, but often just `YYYY-MM-DD` will be enough unless you need the additional precision.
- Next is the `COG_PATH`, which is a URI pointing to a COG somewhere that you're capable of reading. You have to be able to read from the location because the script reads some metadata from the COG to figure out geographic information about the item.
- The last required field is the `COLLECTION_ID`. Items in STAC APIs must be in collections, and this ID indicates which collection this item belongs in.

There are some other optional fields as well:

- `--eo-data` takes a path to an object with an `eo:bands` key. You can see an example in [`./planet-eo.json`](./planet-eo.json). These JSON documents are analogous to datasources in Raster Foundry.
- `--cloud-cover` takes a float estimating the quantity of cloud cover in the image. You don't have to provide this value, but it might be valuable for searching for items later, once you have a lot of them.

`--api-host` and `--api-scheme` serve the same purpose here as in the `create_collection.py` script.

For a small example, you could create an item in the collection you created in the previous section like:

```bash
$ python create_item.py my-great-item 2021-02-01 ./test/data/mask-cog.tif my-great-collection --eo-data ./planet-eo.json --cloud-cover 10
```

Note that `./test/data/mask-cog.tif` won't be accessible to your local Franklin instance, but this example shows at least how to use the script.