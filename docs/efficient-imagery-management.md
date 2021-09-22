# Efficient imagery management with COGs

This document outlines the steps needed to prepare imagery for efficient tiling
with COGs. It assumes you have access to a running Franklin instance and a GDAL
version >= 3.0 available (this is necessary for access to the COG output driver,
which we'll rely on in some examples below). In short, bigger (larger area) COGs
are better and faster than collections of smaller COGs.

## The cloud-optimized GeoTIFF model

Cloud-optimized GeoTIFFs are at their core GeoTIFFs with special headers and a
few requirements:

- they are _internally tiled_, meaning contiguous chunks of the data are
  organized into tiles of the imagery instead of strips
- they have _overviews_, which are collections of lower resolution views of the
  data

The special headers include pointers to exactly how far in the image you have to
go to get to a specific tile in a specific overview.

The cloud-optimized part refers to those headers and their efficiency in server
settings that support HTTP Get range requests. A server can serve the `bar.tiff`
resource at `http://foo.com/bar.tiff`. If `bar.tiff` is a COG, and the server
supports range requests, then a client can:

- make a metadata request to `bar.tiff` to read the metadata explaining where
  all of the specific tiles live
- jump to the special part of `bar.tiff` that the client is interested in

We relied on this capability in both Raster Foundry and Franklin to store source
imagery in a format that's good for humans (it's just GeoTIFF! You can load it
up in QGIS!) and good for the tiling service because it could read less data to
serve a tile.

## Mosaics of COGs

The above works _really nicely_ for individual images. However, in the mosaic
story it gets a little bit more complicated. A _mosaic_ here will refer to
collections of GeoTIFFs or other imagery that should all be queried to determine
what a visualization should look like. In this case, we'll work with one of
Sentinel-2 images from the transition to Franklin demo.

From the repo root, you can download it with the following bash command. You can
skip the `AWS_PROFILE` step if your default profile is named `default`.

```bash
$ export AWS_PROFILE=a-profile-name
$ mkdir data && aws --profile raster-foundry s3 cp \
    s3://rasterfoundry-production-data-us-east-1/demo-cogs/s2-canary-islands-rgb-cog.tif \
    ./data/
```

If you inspect that tiff with `gdalinfo`, you'll see some information about overviews at the bottom:

```bash
$ gdalinfo data/s2-canary-islands-rgb-cog.tif
...
Band 1 Block=256x256 Type=UInt16, ColorInterp=Gray
  Overviews: 6656x6784, 3328x3392, 1664x1696, 832x848, 416x424, 208x212
  Mask Flags: PER_DATASET ALPHA
Band 2 Block=256x256 Type=UInt16, ColorInterp=Undefined
  Overviews: 6656x6784, 3328x3392, 1664x1696, 832x848, 416x424, 208x212
  Mask Flags: PER_DATASET ALPHA
Band 3 Block=256x256 Type=UInt16, ColorInterp=Undefined
  Overviews: 6656x6784, 3328x3392, 1664x1696, 832x848, 416x424, 208x212
  Mask Flags: PER_DATASET ALPHA
Band 4 Block=256x256 Type=UInt16, ColorInterp=Alpha
  Overviews: 6656x6784, 3328x3392, 1664x1696, 832x848, 416x424, 208x212
```

These `Overviews: ` sections indicate the total size of the image in each
overview. Each overview covers the same area, but at lower resolutions, and each
is also tiled into 256x256 pixel tiles. The full resolution for this image is
about 10m, so the overviews cover 20m, 40m, 80m, 160m, 320m, and 640m / pixel
resolutions. If you request a TMS tile at a zoom level where a pixel is at or
below 640m, a COG-friendly server can just read the 208x212 pixels for the last
overview. That requires reading 44096 pixels.

Now let's talk about what happens if we split this image up. We can divide this
image up into a smaller images using the `gdal_retile.py` helper script. In this
case, I split it up and COGified the splits in a single command.

```bash
$ mkdir data/retiled/ && \
    gdal_retile.py \
    -of COG \
    -co COMPRESS=DEFLATE \
    -ps 2500 2500 \
    -targetDir ./data/retiled/ \
    data/s2-canary-islands-rgb-cog.tif
```

This command will create a bunch of sections of the source tif that are
COGified slices of the full tif. Let's inspect one of them:

```bash
$ gdalinfo data/retiled/s2-canary-islands-rgb-cog_1_1.tif
...
Band 1 Block=512x512 Type=UInt16, ColorInterp=Gray
  Overviews: 1250x1250, 625x625, 312x312
Band 2 Block=512x512 Type=UInt16, ColorInterp=Undefined
  Overviews: 1250x1250, 625x625, 312x312
Band 3 Block=512x512 Type=UInt16, ColorInterp=Undefined
  Overviews: 1250x1250, 625x625, 312x312
Band 4 Block=512x512 Type=UInt16, ColorInterp=Undefined
  Overviews: 1250x1250, 625x625, 312x312
```

The source resolution hasn't changed, so now we have overviews for 20, 40, and
80 meters for the sides of each pixel.

Let's consider the low zoom case where a single TMS tile request covers all of
data and the desired pixel size is about 640m. Assume that the source image is
perfectly tiled in the sense that the tiles here cover exactly the same area. We
have a 6x6 grid of tiles that contribute to the image. For each tile, the
smallest overview we have is 312x312. The server will have to read all of that
data and downsample it server-side by a factor of 8. So, for each of the 36
tiles, we have to read a 312x312 grid. Now, to create the same image as the
single image case, we have to read 36 * 312 * 312 pixels, or 3,504,384 pixels.
That's about 80x as much data as we had to read in the single image case, _and_
we have to do work after the read to resample the imagery and combine the data
from each tiff.

## Managing imagery efficiently in Franklin

This isn't to say that all mosaics are bad and you shouldn't use them. Sometimes
mosaics make sense. However, if you find yourself in a situation where the
server appears to be significantly slower than makes sense, one bit of recourse
you can try is to combine some of the images that contribute.

You can combine COGs into larger COGs using `gdalbuildvrt` and `gdal_translate`.

For example, any time you have several COGs in the same directory, you can run
these three commands to get a single COG from all of the contributing images:

```bash
# build a virtual raster from the contributing rasters
$ gdalbuildvrt data/output.vrt data/retiled/*.tif
# add overviews using the default overview selection heuristics
$ gdaladdo data/output.vrt
# gather the data from all of the contributing rasters into a single larger COG
$ gdal_translate -of COG -co COMPRESS=DEFLATE data/output.vrt data/cogified-vrt.tiff
```

You can run `gdalinfo data/cogified-vrt.tiff` to verify that we've recovered the
overviews down to 208x212.

Then, since Franklin creates mosaics from specific items, you can create a new
item pointing to your combined and COGified tiff, a new mosaic definition
including only that item, and then point to the specific tile URL for that
mosaic in the client program or interface.