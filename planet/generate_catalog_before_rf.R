library(sf)
library(yaml)
library(aws.s3)
library(raster)
library(dplyr)
library(parallel)
library(data.table)
library(rmapaccuracy)

# Load the config yaml file
params <- yaml.load_file("cfg/config.yaml")

# Read grids tile file
grids_tile_path <- file.path(params$imagery$catalog_path,
                             params$imagery$grids_tile_filename)
grids_tile <- read.csv(grids_tile_path)

# Filter the grids if set
if (isTRUE(params$imagery$tile_filter)) {
  filter_path <- file.path(params$imagery$catalog_path,
                           params$imagery$tile_filter_file)
  qsites <- read.csv(filter_path)
  if (names(qsites) != "tile") names(qsites) <- "tile"
  grids_tile <- merge(qsites, grids_tile, by = "tile")
}

# Read the scenes in s3
items <- get_bucket(bucket = params$imagery$s3_bucket, 
                    prefix = params$imagery$s3_composite_prefix, 
                    max = Inf)
keys <- lapply(c(2: length(items)), function(i) {
  items[[i]]$Key
})
scenes <- unlist(keys) %>% 
  data.table(scene_id = .) %>%
  filter(stringr::str_detect(scene_id, '.tif'))

# Set coninfo
if (params$database$mode == "production") {
  dbname <- params$database$db_production_name
} else {
  dbname <- params$database$db_sandbox_name
}
con <- DBI::dbConnect(RPostgreSQL::PostgreSQL(), 
                      host = params$database$db_host, 
                      dbname = dbname,   
                      user = params$database$db_username, 
                      password = params$database$db_password)
xy_tabs <- tbl(con, "master_grid") %>% 
  filter(id %in% !!grids_tile$id) %>% 
  dplyr::select(x, y, id) %>% 
  collect()
DBI::dbDisconnect(con)

# Align composite images and other info
num_cores <- min(nrow(scenes), detectCores() - 1)
pre_scenes <- mclapply(1:nrow(scenes), function(i) {
  row_each <- scenes$scene_id[i]
  url <- file.path("s3://activemapper", row_each)
  items <- strsplit(row_each, "/")[[1]]
  season <- items[3]
  scene_id <- gsub(".tif", "", items[4])
  tile_id <- stringr::str_extract(strsplit(scene_id, "_")[[1]][1],
                                  "[0-9]+")
  
  grids <- grids_tile %>% dplyr::filter(tile == tile_id) 
  
  if (nrow(grids) > 0) {
    # xy_tabs <- tbl(con, "master_grid") %>% 
    #   filter(id %in% !!grids$id) %>% 
    #   dplyr::select(x, y) %>% 
    #   collect()
    xy_tabs <- xy_tabs %>%
      filter(id %in% !!grids$id) %>%
      dplyr::select(x, y)
      
    rowcol <- rowcol_from_xy(xy_tabs$x, 
                             xy_tabs$y, 
                             offset = -1) %>%
      data.table()
    
    grids <- grids %>%
      mutate(cell_id = id, 
             scene_id = scene_id,
             col = rowcol$col,
             row = rowcol$row,
             season = season,
             url = url) %>%
      dplyr::select(-c(id, tile, aoi))
    
  }
  grids
}, mc.cores = num_cores)

planet_catalog <- do.call(rbind, pre_scenes)

# Write out the catalog file to register RF
planet_catalog_path <- file.path(params$imagery$catalog_path,
                                 params$imagery$catalog_filename)
write.csv(planet_catalog, 
          planet_catalog_path,
          row.names = F)

# Save the catalog file into S3 for cvml
planet_catalog_s3 <- planet_catalog %>%
  mutate(uri = url) %>%
  dplyr::select(-c(url))
planet_catalog_s3_path <- file.path(params$imagery$s3_catalog_prefix,
                                    paste0("planet_catalog_", 
                                           params$imagery$aoiid,
                                           ".csv"))
s3write_using(planet_catalog_s3, 
              FUN = write.csv,
              row.names = F,
              object = planet_catalog_s3_path, 
              bucket = params$imagery$s3_bucket)