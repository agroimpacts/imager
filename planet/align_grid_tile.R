library(sf)
library(yaml)
library(raster)
library(aws.s3)
library(dplyr)
library(parallel)
library(data.table)

# Load the config yaml file
params <- yaml.load_file("cfg/config.yaml")

# Read the files in need from S3
# including tiles geojson and master grid Geotif
tiles <- s3read_using(st_read, 
                      object = params$imagery$tile_object,
                      bucket = params$imagery$s3_bucket)
master_grid <- s3read_using(raster,
                            object = params$imagery$master_grid_object,
                            bucket = params$imagery$s3_bucket)

# Allign the grids with tiles
num_cores <- min(nrow(tiles), detectCores() - 1)
assign_grids <- mclapply(1:nrow(tiles), function(i) {
  tile_each <- tiles[i, ]
  mst_each <- crop(master_grid, tile_each)
  ids <- getValues(mst_each) %>% 
    data.table(id = .) %>%
    mutate(aoi = tile_each$aoi,
           tile = tile_each$tile)
  ids
  
}, mc.cores = num_cores)
grids_tile <- do.call(rbind, assign_grids)
grids_tile_path <- file.path(params$imagery$catalog_path,
                             params$imagery$grids_tile_filename)
write.csv(grids_tile, grids_tile_path, row.names = F)