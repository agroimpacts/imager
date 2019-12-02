## ---------------------------
## Script name: align_grid_tile.R
##
## Purpose of script: to aline the grids and tiles
## for the study area based on the master_grid.tif
## and tile file e.g. ghana_tiles.geojson.
##
## Author: Lei Song
## Email: ***REMOVED***
## ---------------------------

library(sf)
library(yaml)
library(raster)
library(aws.s3)
library(dplyr)
library(parallel)
library(data.table)

# Load the config yaml file
params <- yaml.load_file("cfg/config.yaml")
gcsstr <- "+proj=longlat +datum=WGS84 +no_defs +ellps=WGS84 +towgs84=0,0,0"

# Read the files in need from S3
# including tiles geojson and master grid Geotif
tiles <- s3read_using(st_read, 
                      object = params$imagery$tile_object,
                      bucket = params$imagery$s3_bucket)
master_grid <- s3read_using(raster,
                            object = params$imagery$master_grid_object,
                            bucket = params$imagery$s3_bucket)
crs(master_grid) <- gcsstr

# Allign the grids with tiles
num_cores <- min(nrow(tiles), detectCores() - 1)
assign_grids <- mclapply(1:nrow(tiles), function(i) {
  tile_each <- tiles[i, ]
  mst_each <- crop(master_grid, tile_each)
  ids <- getValues(mst_each) %>% 
    data.table(id = .) %>%
    na.omit() %>%
    mutate(aoi = tile_each$aoi,
           tile = tile_each$tile)
  ids
  
}, mc.cores = num_cores)

# The boundary is not regular, so there might be some NAs
grids_tile <- do.call(rbind, assign_grids) %>% na.omit()

## Add lines to query the neighborhoods of a tile
groups <- st_intersects(tiles, tiles)
clustering <- mclapply(1:length(groups), function(i) {
  index <- unlist(groups[i])
  tile <- tiles[i, ]$tile
  tile_ng <- tiles[index, ]$tile
  tile_ng <- paste(tile_ng, collapse=",")
  data.table(tile = tile, tile_ng = tile_ng)
}, mc.cores = num_cores)
clusters <- do.call(rbind, clustering)
grids_tile <- merge(grids_tile, clusters, by = "tile")

grids_tile_path <- file.path(params$imagery$catalog_path,
                             params$imagery$grids_tile_filename)
write.csv(grids_tile, grids_tile_path, row.names = F)
