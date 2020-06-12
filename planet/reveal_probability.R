## ---------------------------
## Script name: reveal_probability.R
##
## Purpose of script: to register probability tiles to Raster Foundary
## and build projects for differnet aois, then 
## save out a csv including basic information.
##
## Author: Lyndon Estes and Lei Song
## ---------------------------
library(dplyr)
library(aws.s3)
library(raster)
library(sf)
library(glue)
library(stringr)
library(aws.s3)

###################### Align grids, tiles and grps ##################
# get credentials: point the path to where your config.yaml file lives
params <- yaml::yaml.load_file("cfg/config.yaml")
dinfo <- params$database
con <- DBI::dbConnect(RPostgreSQL::PostgreSQL(), host = dinfo$db_host, 
                      dbname = "Africa", user = dinfo$db_username, 
                      password = dinfo$db_password)
# qsites
scenes <- tbl(con, "scenes_data") %>% collect()
ids <- scenes %>% distinct(cell_id) %>% pull()
sites <- tbl(con, "master_grid") %>% 
  filter(id %in% ids) %>% dplyr::select(id, name, x, y) %>% 
  collect()
# master_grid dimensions
mgrid <- raster::raster("/vsis3/***REMOVED***/grid/master_grid.tif")
tile_grid <- raster(extent(mgrid), res = 0.05)
# sites
sites <- sites %>% 
  mutate(mgridx = colFromX(mgrid, x) - 1,
         mgridy = rowFromY(mgrid, y) - 1,
         tilex = colFromX(tile_grid, x) - 1,
         tiley = rowFromY(tile_grid, y) - 1) %>% 
  mutate(grp = case_when(
    y < -1 ~ "1",
    between(y, -1, 0) ~ "2", 
    between(y, 0.9, 1.6) ~ "3",
    y > 1.7 ~ "4" 
  ))
tms_sites <- left_join(scenes, sites, by = c("cell_id" = "id"))
grps <- tms_sites %>% dplyr::select(tilex, tiley,
                                    grp) %>% distinct() %>%
  mutate(prob_path = paste0("classified-images/congo_whole_cog/image_c", 
                            tilex, "_r", tiley, ".tif"))

################## List all generated probability imagery ################
# Read the imagery in s3
prefix <- paste0(params$probability$s3_cogtiff_perfix, "/")
items <- get_bucket(bucket = params$probability$s3_bucket, 
                    prefix = prefix, 
                    max = Inf)
keys <- lapply(c(1: length(items)), function(i) {
  items[[i]]$Key
})
pros <- unlist(keys)

# Tailor grps
grps <- grps %>%
  filter(prob_path %in% pros) %>%
  mutate(prob_id = str_extract(prob_path, "image_c[0-9]+_r[0-9]+"))

# Register probability tiles
# Could use mclapply to spead up
register_prob <- lapply(1:nrow(grps), function(i){
  prob_path <- grps[i, ]$prob_path
  bucket <- params$probability$s3_bucket
  tile_id <- gsub(".tif", "", strsplit(prob_path, "/")[[1]][3])
  url <- glue("s3://{bucket}/{prob_path}")
  cmd_text <- glue(paste0("python3.6 rasterfoundry_register_cls.py ",
                          "--scene_id {tile_id} --url {url}"))
  prob_rf_id <- system(cmd_text, intern = TRUE)
  data.frame(prob_id = tile_id, prob_rf_id = prob_rf_id)
})
prob_rf_unique <- do.call(rbind, register_prob)

# Create projects for aois
grps <- merge(grps, prob_rf_unique, by = "prob_id")

# Build project
build_project <- lapply(unique(grps$grp), function(i) {
  project_id <- glue("probability_aoi{i}")
  grps_sub <- grps %>% filter(grp == i)
  
  prob_rf_ids <- paste(grps_sub$prob_rf_id, collapse = ",")
  cmd_text <- sprintf(paste0("python3.6 rasterfoundry_create_project.py ",
                             "--scene_id %s ",
                             "--scene_ids %s"),
                      project_id, 
                      prob_rf_ids)
  tms_url <- system(cmd_text, intern = TRUE)
  data.frame(project_id = project_id, 
             tms_url = tms_url)
})
probability_projects <- do.call(rbind, build_project)
s3write_using(probability_projects, 
              FUN = write.csv,
              row.names = F,
              object = params$probability$s3_catalog_prob, 
              bucket = params$probability$s3_bucket)
