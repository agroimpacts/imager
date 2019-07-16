import argparse
import csv
import yaml
import codecs
from rf_client import *

# read config
with open("cfg/config.yaml", 'r') as yaml_file:
    config = yaml.load(yaml_file)
imagery_config = config['imagery']

# extra csv input params
csv_output_encoding = imagery_config['output_encoding']
csv_header = ["cell_id", "scene_id", "col", "row", "season", "url", "tms_url"]

# rfclient init
rfclient = RFClient(config)

# psql client init
# psqlclient = PSQLPClient(config)
# psqlclient.connect()

def main(csv_path, psql_mode):
    if (psql_mode != None) and eval(psql_mode):
        curs = psqlclient.query_without_tms_url()
        for row in curs:
            (provider, scene_id, cell_id, season, global_col, global_row, url, tms_url, date_time) = row
            tms_uri = rfclient.create_tms_uri(scene_id, url)
            psqlclient.insert_rows_by_one_async([(provider, scene_id, cell_id, season, global_col, global_row, url, tms_uri)])

        curs.close()        
    else:
        # if not pased as an argument, pick it up from the config file
        if csv_path == None:
            config = configparser.ConfigParser()
            config.read('cfg/config.ini')
            csv_path = config['imagery']['output_filename']

        csv_path_out = "{}.out.csv".format(csv_path)

        # output CSV file, always overwrite as it's a safe copy
        fp = codecs.open(filename = csv_path_out, mode = 'w', encoding = csv_output_encoding) # buffering = 20*(1024**2)
        writer = csv.writer(fp)
        writer.writerow(csv_header)

        reader = csv.reader(open(csv_path))
        next(reader, None)  # skip headers
        # cell_id,scene_id,col,row,season,url,tms_url
        for line in reader:
            (cell_id, scene_id, x, y, season, url, tms_url) = line
            tms_uri = rfclient.create_tms_uri(scene_id, url)
            writer.writerow([cell_id, scene_id, x, y, season, url, tms_uri])
            # psqlclient.insert_rows_by_one_async([('planet', scene_id, cell_id, season, x, y, url, tms_uri)])

        fp.close()    

    # psqlclient.drain()
    # psqlclient.close()
    # psqlclient.connection_close()
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = 'Generate new TMS links for the given csv file')
    parser.add_argument('--path', help='path to a local csv file, optional, if not set would pick it up from the config file')
    parser.add_argument('--psql', help='mode to read data, pass True to work with PSQL only')
    args = parser.parse_args()
    main(args.path, args.psql)
