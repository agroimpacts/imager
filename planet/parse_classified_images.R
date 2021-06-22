library(aws.s3)
library(dplyr)
library(data.table)
library(raster)
library(sf)

gcsstr <- "+proj=longlat +datum=WGS84 +no_defs +ellps=WGS84 +towgs84=0,0,0"
tiles_homemade <- raster(nrows=1460, ncols=1380, 
                         xmn=-17.541, xmx=51.459, 
                         ymn=-35.46, ymx=37.54, 
                         crs = gcsstr, 
                         vals = 1:(1380*1460))

aois <- s3read_using(st_read, 
                     quiet = TRUE,
                     object = "grid/image_target_aois.geojson",
                     bucket = "activemapper")

ids_instance <- 1:16
perfix_s3 <- paste(ids_instance, "whole", sep = "_")

gather_data <- lapply(perfix_s3, function(x) {
  # Read the scenes in s3
  items <- get_bucket(bucket = "activemapper", 
                      prefix = file.path("classified-images", x), 
                      max = Inf)
  keys <- lapply(c(1:length(items)), function(i) {
    cls_id <- strsplit(items[[i]]$Key,"/")[[1]][3]
    url <- file.path("s3://activemapper", items[[i]]$Key)
    aoi_id <- x
    data.table(cls_id = cls_id,
               aoi_id = aoi_id,
               url = url)
  })
  
  tiles <- do.call(rbind, keys) %>% 
    filter(stringr::str_detect(url, "_whole/"))
  
  idx <- gsub("_whole", "", x)
  aois_idx <- aois[idx, ]
  aois_tile <- mask(crop(tiles_homemade, aois_idx), aois_idx)
  aois_tile <- na.omit(getValues(aois_tile))
  cls_ids <- lapply(aois_tile, function (i) {
    r <- i%/%1380
    c <- i - r * 1380 - 1
    paste0("image_c", c, "_r", r, ".tif")
  }) %>% unlist()
  
  tiles %>%
    mutate(grid = ifelse(cls_id %in% cls_ids, "inside", "outside"))
})

tiles_cls <- do.call(rbind, gather_data)
write.csv(tiles_cls, 
          "catalog/tiles_cls.csv",
          row.names = F)
