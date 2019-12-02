## ---------------------------
## Script name: downloader_functions.R
##
## Purpose of script: a list of functions to 
## run command lines to deliver the Planet images to S3. 
## More details are in python package porder.
##
## Author: Lei Song
## Email: ***REMOVED***
## ---------------------------

## Load the packages
library(aws.s3)
library(raster)
library(sf)
library(yaml)
library(rgdal)
library(rgeos)
library(dplyr)
library(data.table)
library(tidyverse)
library(parallel)
library(lubridate)

## idlist_img to query the list of ids of the Planet imagery for an AOI with super grids
idlist_img <- function(params = NULL) {
  if (is.null(params)) {
    stop("Must set a params yaml object to read settings!")}
  
  ## Parse settings from params yaml
  aoi <- s3read_using(st_read, 
                      object = params$imagery$tile_object,
                      bucket = params$imagery$s3_bucket)
  prefix_geojson <- params$imagery$prefix_geojson
  geojson_folder <- params$imagery$geojson_folder
  orderlist_folder <- params$imagery$orderlist_folder
  start_date <- params$imagery$start_date
  end_date <- params$imagery$end_date
  item <- params$imagery$item
  asset <- params$imagery$asset
  filter <- params$imagery$filter
  mode <- params$imagery$mode
  
  ## Check the paths and create them
  cat("Checking and creating paths...\n")
  geojson_path <- file.path(getwd(), 
                            params$imagery$catalog_path, 
                            geojson_folder)
  orderlist_path <- file.path(getwd(), 
                              params$imagery$catalog_path, 
                              orderlist_folder)
  dir.create(geojson_path, showWarnings = FALSE)
  dir.create(orderlist_path, showWarnings = FALSE)
  
  ## Use the settings to query the list of ids using Planet API and package porder
  cat("Querying the id of the Planet imagery for each supergrid...\n")
  numCores <- min(nrow(aoi), detectCores() - 1)
  multi_query <- mclapply(c(1:nrow(aoi)), function(i) {
    aoi_each <- aoi[i, ]
    geojson_name <- paste(prefix_geojson, 
                          aoi_each$tile, sep = "_")
    path <- file.path(geojson_path, 
                      sprintf("%s.geojson", geojson_name))
    st_write(aoi_each, path, delete_dsn = TRUE, quiet = TRUE)
    path_csv <- file.path(orderlist_path, 
                          sprintf("orderlist_%s.csv", geojson_name))
    
    ## Run command line to order Planet imagery
    if (isTRUE(filter)){
      query_text <- sprintf(paste0("porder idlist --input '%s' ",
                                   "--start '%s' --end '%s' --item '%s' ",
                                   "--asset '%s' ",
                                   "--number 1000 ",
                                   "--outfile '%s' ",
                                   ## May 29, 2019, add the filter option
                                   "--filter 'range:view_angle:-3:3' ",
                                   "'string:ground_control:True' ", 
                                   "'string:quality_category:standard'"), 
                            path, start_date, 
                            end_date, item, 
                            asset, path_csv)      
    } else {
      query_text <- sprintf(paste0("porder idlist --input '%s' ",
                                   "--start '%s' --end '%s' --item '%s' ",
                                   "--asset '%s' ",
                                   "--number 1000 ",
                                   "--outfile '%s'"), 
                            path, start_date,
                            end_date, item, 
                            asset, path_csv)
    }
    
    cmd_run <- system(query_text, intern = TRUE)
    ## Arbitrary checking
    if (!grepl("^Total", cmd_run[3])) {
      stop("Something wrong with the query, better to check.")
    }
    
    ids_aoi_each <- read.csv(path_csv, 
                             col.names = "id", 
                             stringsAsFactors = F, 
                             header = F)
    if(nrow(ids_aoi_each) > 0) {
      ids_aoi_each$aoi <- aoi_each$aoi
      ids_aoi_each$tile <- aoi_each$tile
    }
    ids_aoi_each
  }, mc.cores = numCores)
  
  if (!mode %in% c(1, 2)) {stop("mode should be 1 or 2!")}
  if (mode == 1){
    cat("Writing out the csv of catalog and orderlist...\n")
    orderlist_all <- do.call(rbind, multi_query)
    catalog_file <- file.path(orderlist_path, 
                              paste0("catalog_", 
                                     prefix_geojson, ".csv"))
    write.table(orderlist_all, catalog_file, row.names = F)
    
    ## Arbitrary saving, should parse the config yaml eventually
    put_object(file = catalog_file, 
               object = file.path(params$imagery$catalog_composite_s3,
                                  paste0("catalog_tiles_", 
                                         prefix_geojson, ".csv")))        
  }
  
  ## Return the id list 
  unique(orderlist_all$id)
  cat("Finish the idlist! It is good to go to the download_img now!\n")
}

download_img <- function(ids_all = NULL,
                         params = NULL) {
  ## Check the inputs
  cat("Checking the inputs...\n")
  if (is.null(params)) {
    stop("Must set a params yaml object to read settings!")
  } else if (is.null(ids_all)) {
    stop("Must set a ids_all to continue!")
  }
  
  # Parse settings
  prefix_geojson <- params$imagery$prefix_geojson
  orderlist_path <- params$imagery$orderlist_folder
  item <- params$imagery$item
  asset <- params$imagery$asset
  aws_cred <- params$imagery$aws_cred
  aws_cred <- file.path(getwd(), aws_cred)
  
  # Make folder
  orderlist_path <- file.path(getwd(), 
                              params$imagery$catalog_path, 
                              orderlist_folder)
  dir.create(orderlist_path, showWarnings = FALSE)
  if(!file.exists(aws_cred)) {stop("Cannot find the aws credential yml file!")}
  
  cat("Ordering the imagery in the orderlist...\n")
  sub_lists <- suppressWarnings(split(ids_all, 
                                      (0:length(ids_all) %/% 2000)))
  numCores <- min(length(sub_lists), detectCores() - 1)
  order_img <- mclapply(c(1:length(sub_lists)), function(i) {
    ids_all <- sub_lists[[i]]
    orderlist_all_path <- file.path(orderlist_path, 
                                    paste0("orderlist_", 
                                           prefix_geojson,
                                           "_group",i,".csv"))
    write.table(ids_all, orderlist_all_path, 
                col.names = F, row.names = F)
    
    ## Run command line to download the imagery
    order_text <- sprintf(paste0("porder order --name '%s_order' ",
                                 "--idlist '%s' ",
                                 "--item '%s' ",
                                 "--asset '%s' ",
                                 "--aws '%s' ",
                                 "--op aws"),
                          prefix_geojson, orderlist_all_path, 
                          item, asset, aws_cred)
    system(order_text, intern = TRUE)
    # order_link <- system(order_text, intern = TRUE)
    # order_link <- strsplit(order_link, " ")[[1]][4]
    # order_id <- strsplit(order_link, "/")[[1]][8]
    
  }, mc.cores = numCores)
  
  cat("Finish delivery!!\n")
}

## WARNING: This function is designed 
## specifically for Planet images delivered by porder to S3
diff_s3 <- function(ids_all = NULL,
                    bucket = "***REMOVED***",
                    prefix = "planet/analytic_sr_all") {
  
  ## Get the keys in S3
  cat("Extracting the keys in S3...\n")
  items <- get_bucket(bucket, prefix = prefix, max = Inf)
  keys <- lapply(c(2: length(items)), function(i) {
    items[[i]]$Key
  })
  keys <- unlist(keys)
  keys <- grep("AnalyticMS_SR.tif", keys, value = T)
  keys <- unique(str_extract(keys, "[0-9]{8}_[0-9]+(_1{1})?_[a-zA-Z0-9]+"))
  
  ## Get the ids remaining need to download
  setdiff(ids_all, keys)
}

## WARNING: This function is designed specifically 
## for Planet images delivered by porder to S3
## Rarely used, because of a mistake happend before 
## due to the unusual names like '20180826_102856_1_0f3b'
reduce_duplicate_s3 <- function(bucket = "***REMOVED***",
                                prefix = "planet/analytic_sr_all") {
  
  ## Get the keys in S3
  ## It takes time if the number of items is large
  cat("Extracting the keys in S3...\n")
  items <- get_bucket(bucket, prefix = prefix, max = Inf)
  keys <- lapply(c(2: length(items)), function(i) {
    items[[i]]$Key
  })
  keys <- unlist(keys)
  keys <- grep("AnalyticMS_SR.tif", keys, value = T)
  
  ids <- str_extract(keys, "[0-9]{8}_[0-9]+(_1{1})?_[a-zA-Z0-9]+")
  
  ## Get the duplicates
  ids_dup <- ids[duplicated(ids)]
  keys_id <- data.table(keys) %>%
    mutate(id = str_extract(keys,
                            "[0-9]{8}_[0-9]+(_1{1})?_[a-zA-Z0-9]+")) %>%
    filter(id %in% ids_dup) %>%
    arrange(id)
  
  ## Pick the duplicates 
  keys_id <- keys_id[duplicated(keys_id$id), ]
  
  ## Remove them from S3
  cat("Deleting the duplicates...\n")
  numCores <- min(nrow(keys_id), detectCores() - 1)
  mclapply(keys_id$keys, function(key_each) {
    delete_object(object = key_each, bucket = bucket)
    
  }, mc.cores = numCores)
  cat("Bucket is clean now!\n")
}
