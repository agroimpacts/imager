#!/usr/bin/env Rscript
library(yaml)
library(optparse)
source("downloader_functions.R")

## Main function to download
downloading <- function(){
  yaml_file <- "cfg/config.yaml"
  if(!file.exists(yaml_file)) {stop("Cannot find the yaml file!")}
  params <- yaml.load_file(yaml_file)
  
  # Get settings
  clean <- params$imager$clean
  bucket <- params$imagery$s3_bucket
  prefix <- params$imagery$s3_analytic_sr_prefix
  
  ## Query the id list
  ids_all <- idlist_img(params = params)
  
  repeat{
    ## Check the S3
    ## Only run reduce_duplicate_s3() as needed
    if(clean) {reduce_duplicate_s3(bucket = bucket,
                                   prefix = prefix)}
    ids_all <- diff_s3(ids_all,
                       bucket = bucket,
                       prefix = prefix)
    
    ## Download the imageries
    if(length(ids_all) > 0){
      download_img(ids_all = ids_all,
                   params = params)
    }
    else {
      cat("All images are downloaded!\n")
      cat("Cleaning the interim folders")
      geojson_folder <- params$imagery$geojson_folder
      orderlist_folder <- params$imagery$orderlist_folder
      geojson_path <- file.path(getwd(), 
                                params$imagery$catalog_path, 
                                geojson_folder)
      orderlist_path <- file.path(getwd(), 
                                  params$imagery$catalog_path, 
                                  orderlist_folder)
      unlink(geojson_path)
      unlink(orderlist_path)
      
      quit("Finish", save = "no")
    }
    
    ## Need time to delivery the data to AWS S3
    Sys.sleep(3600)
  }
  
}

## Run
downloading()