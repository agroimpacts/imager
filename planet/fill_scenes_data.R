library(sf)
library(DBI)
library(yaml)
library(aws.s3)
library(raster)
library(dplyr)
library(parallel)
library(data.table)
library(rmapaccuracy)

# Load the config yaml file
params <- yaml.load_file("cfg/config.yaml")

# Read catalog file
planet_catalog_path <- file.path(params$imagery$catalog_path,
                                 params$imagery$catalog_filename)
planet_catalog <- read.csv(planet_catalog_path)

## Register only once for the same image
imgs <- unique(planet_catalog$url) %>% 
                  data.table(url = .)
imgs_tb <- merge(imgs, planet_catalog[, c("scene_id", "url")], 
                 by = "url") %>%
                  distinct()
num_cores <- min(nrow(imgs_tb), detectCores() - 1)
get_tms_url <- mclapply(1:nrow(imgs_tb), function(i){
  scene_id <- imgs_tb[i, ]$scene_id
  url <- imgs_tb[i, ]$url
  cmd_text <- sprintf(paste0("python3 rasterfoundry_register_each.py ",
                     "--scene_id %s ",
                     "--url %s"),
                     scene_id, url)
  tms_url <- system(cmd_text, intern = TRUE)
  data.table(scene_id = scene_id, tms_url = tms_url)
}, mc.cores = num_cores)
planet_catalog_unique <- do.call(rbind, get_tms_url)
planet_catalog <- merge(planet_catalog, planet_catalog_unique, by = "scene_id")

# Updates the database
# Clean the old ones
coninfo <- mapper_connect(host = params$database$db_host,
                          user = "sandbox")
scenes_data <- tbl(coninfo$con, "scenes_data") %>% 
                  filter(cell_id %in% !!planet_catalog$cell_id) %>% 
                  dplyr::select(cell_id) %>% 
                  collect()

cell_ids <- paste0("'", scenes_data$cell_id, "'", collapse = ",")
sql <- paste0("DELETE FROM scenes_data WHERE cell_id in (", cell_ids, ")")
dbExecute(coninfo$con, sql)
db_commit(coninfo$con)

# Insert new ones
insert_db <- lapply(1:nrow(planet_catalog), function(i) {
                  row_catalog <- planet_catalog[i, ]
                  # "cell_id", "scene_id", "col", "row", "season", "url", "tms_url"
                  sql <- sprintf(paste0("insert into scenes_data",
                                        " (provider, scene_id, cell_id, season, ",
                                        "global_col, global_row, ",
                                        "url, tms_url, date_time) values",
                                        " ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');"),
                                 "planet", row_catalog$scene_id, row_catalog$cell_id,
                                 row_catalog$season, row_catalog$col, row_catalog$row, 
                                 row_catalog$url, row_catalog$tms_url, Sys.time())
                  dbExecute(coninfo$con, sql)
                  # db_commit(coninfo$con)
})
