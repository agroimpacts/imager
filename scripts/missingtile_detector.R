library(aws.s3)
library(sf)
library(dplyr)

common_path <- "/some/path/"
params <- yaml::yaml.load_file(file.path(common_path, 'cvmapper_config_composite.yaml'))
Sys.setenv("AWS_ACCESS_KEY_ID" = params$cvml$aws_access,
           "AWS_SECRET_ACCESS_KEY" = params$cvml$aws_secret,
           "AWS_DEFAULT_REGION" = params$cvml$aws_region)

key.list <- get_bucket_df(bucket = '***REMOVED***', prefix = 'planet/composite_sr_buf_fix/GS', max = Inf)['Key']
key.list.subset <- unlist(lapply(1:nrow(key.list), function(x)
  substring(key.list$Key[x], 36,41) # need to check here in case that the planet directory change
))

ghana.tiles <- st_read(file.path(common_path, 'ghana_tiles_merged.geojson'))$tile

ghana.geojson <- st_read(file.path(common_path, 'ghana_tiles_merged.geojson'))

'%nin%' <- Negate('%in%')

diff.gs <- ghana.tiles[ ghana.tiles %nin% key.list.subset]


key.list <- get_bucket_df(bucket = '***REMOVED***', prefix = 'planet/composite_sr_buf_fix/OS', max = Inf)['Key']

key.list.subset <- unlist(lapply(1:nrow(key.list), function(x)
  substring(key.list$Key[x], 36,41)
))
'%nin%' <- Negate('%in%')

diff.os <- ghana.tiles[ ghana.tiles %nin% key.list.subset]

diff.union <- union(diff.os, diff.gs)

ghana.tiles.unfinished <- ghana.geojson[ghana.geojson$tile %in% diff.union, ]

fourmonth.count <- ghana.geojson %>% filter(production_aoi %in% c(10, 11, 13, 14, 16)) 
print(paste0("Four month OS processing tile should be ", nrow(fourmonth.count)))
print(paste0("unfinished os is ", length(diff.os), "; unfinished gs is ", length(diff.gs)))
print(paste0("The union unfinished count is ", length(diff.union)))
print(paste0("Unprocessed four month os is ", length(diff.union[diff.union %in% fourmonth.count$tile])))
##################################################################################
#                              the below is to double check                     #
##################################################################################

key.list <- get_bucket_df(bucket = '***REMOVED***', prefix = 'planet/composite_sr_buf_fix/OS', max = Inf)['Key']

key.list.subset <- unlist(lapply(1:nrow(key.list), function(x)
  substring(key.list$Key[x], 43,48)
))

print(paste0("The four month OS has been processed ", length(key.list.subset[key.list.subset==736999])))
print(paste0("The three month OS has been processed ", length(key.list.subset[key.list.subset==737029])))

length(key.list.subset[key.list.subset==736999]) + length(key.list.subset[key.list.subset==737029]) + length(diff.os)


#### save csv
interval <- length(diff.union)/4
# write.table(diff.union, file = file.path(common_path, 'missingtiles_1123019_3.csv'),col.names='tile', sep=",")

write.table(diff.union[1:interval], file = file.path(common_path, 'missingtiles_1123019_1.csv'),col.names='tile', sep=",")
write.table(diff.union[(interval + 1): (2 * interval)], file = file.path(common_path, 'missingtiles_1123019_2.csv'),col.names='tile', sep=",")
write.table(diff.union[(2*interval + 1): (3 * interval)], file = file.path(common_path, 'missingtiles_1123019_3.csv'),col.names='tile', sep=",")
write.table(diff.union[(3*interval+1): (4 * interval)], file = file.path(common_path, 'missingtiles_1123019_4.csv'),col.names='tile', sep=",")
# remainingfiles <- read.csv(file.path(common_path,'RemainingTilesCSV.csv'), header=FALSE)$V1
# remainingfiles.subset <- unlist(lapply(1:length(remainingfiles), function(x)
#   substring(remainingfiles[x], 4, 10)
# ))
# 
# '%nin%' <- Negate('%in%')
# 
# diff.gs <- remainingfiles.subset[ remainingfiles.subset %nin% key.list.subset]

key.list.os <- get_bucket_df(bucket = '***REMOVED***', prefix = 'planet/composite_sr_buf/OS', max = Inf)['Key']
key.list.subset <- unlist(lapply(1:nrow(key.list.os), function(x)
  substring(key.list.os$Key[x], 39, 44) # need to check here in case that the planet directory change
))
length(key.list.subset[key.list.subset==736999])


