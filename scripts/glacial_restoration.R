library(stringr)
library(dplyr)
library(lubridate)
library(readtext)

interval_1 <- interval(ymd('2018-05-01'), ymd('2018-09-30'))
interval_2 <- interval(ymd('2018-11-01'), ymd('2019-02-28'))

# as.is is important here to avoid read the data as factors
url_planet_4bands <- read.csv('url_planet_4bands.csv', as.is = TRUE)
string_format <- readtext('cmd_format.rtf')

# url_planet_4bands_date <- url_planet_4bands %>% mutate(date_loc=na.omit(str_locate(url, c("2017", "2018", "2019")))[1]) 
selected_rows <- lapply(1:nrow(url_planet_4bands), function(x){
  print(x)
  foc_url <- url_planet_4bands$url[x]
  date_loc=na.omit(str_locate(foc_url, c("2017", "2018", "2019")))
  date = as.Date(substr(foc_url, date_loc[1], date_loc[1] + 7), format="%Y%m%d")
  if(is.na(date)){
    date = as.Date(substr(foc_url, date_loc[2], date_loc[2] + 7), format="%Y%m%d")
  }
  if(date %within% interval_1 == TRUE || date %within% interval_2 == TRUE){
    foc_url
  }
})

selected_rows_udm <- lapply(1:nrow(url_planet_4bands), function(x){
  # print(x)
  foc_url <- url_planet_4bands$url[x]
  date_loc=na.omit(str_locate(foc_url, c("2017", "2018", "2019")))
  date = as.Date(substr(foc_url, date_loc[1], date_loc[1] + 7), format="%Y%m%d")
  if(is.na(date)){
    date = as.Date(substr(foc_url, date_loc[2], date_loc[2] + 7), format="%Y%m%d")
  }
  if(date %within% interval_1 == TRUE || date %within% interval_2 == TRUE){
    foc_url_new = gsub('SR', 'DN_udm', foc_url)
    if(is.na(foc_url_new))
      print(foc_url)
    foc_url_new 
  }
})

url_planet_4bands_selected_udm <- as.data.frame(do.call(rbind, selected_rows_udm[!is.na(selected_rows_udm)]))
url_planet_4bands_selected <- as.data.frame(do.call(rbind, selected_rows[!is.na(selected_rows)]))
colnames(url_planet_4bands_selected_udm) <- c("url")
colnames(url_planet_4bands_selected) <-c("url")
url_planet_4band <- as.data.frame(rbind(url_planet_4bands_selected, url_planet_4bands_selected_udm))
write.table(url_planet_4band, 'url_planet_4bands_restore.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)

increment <- nrow(url_planet_4bands_selected) /2
 
#write.csv(url_planet_4bands_selected, file = '***REMOVED***/url_planet_4bands_selected.csv')
#write.table(url_planet_4bands_selected, '***REMOVED***/url_planet_4bands_selected.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)
t1<- as.data.frame(url_planet_4bands_selected[1:increment,])
write.table(t1, 'url_planet_4bands_tie1.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)
t2 <- as.data.frame(url_planet_4bands_selected[(increment+1):(increment*2),])
write.table(t2, 'url_planet_4bands_tie2.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)
t3 <- as.data.frame(url_planet_4bands_selected_udm[1:increment,])
write.table(t3, 'url_planet_4bands_tie3.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)
t4 <- as.data.frame(url_planet_4bands_selected_udm[(increment+1):(increment*2),])
write.table(t4, 'url_planet_4bands_tie4.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)

key <- as.data.frame('planet/analytic_sr_all/059c6c3e-7f51-43cf-9228-b7dcfdbcc2a0/100/files/PSScene4Band/20171228_100123_1029/analytic_sr/20171228_100123_1029_3B_AnalyticMS_SR.tif')
write.table(key, 'url_planet_4bands_selected_test.txt', col.names = FALSE, row.names = FALSE, quote = FALSE)


                     
cmd <- paste0('aws s3api restore-object --bucket ***REMOVED*** --key ', key, '--restore-request Days=7,GlacierJobParameters={"Tier"="Bulk"}')
system(cmd)

# aws s3api restore-object --bucket $bucket --key planet/analytic_sr_all/059c6c3e-7f51-43cf-9228-b7dcfdbcc2a0/1/files/PSScene4Band/20171231_105752_0f53/analytic_sr/20171231_105752_0f53_3B_AnalyticMS_SR.tif --restore-request '{"Days":7,"GlacierJobParameters":{"Tier":"Bulk"}}'