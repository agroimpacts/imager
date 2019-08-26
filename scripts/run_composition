#!/bin/bash
composite_exe=/home/ubuntu/imager/python/CompositionMaker.py
config_filenm=cvmapper_config_composite.yaml
threads_num=4
output_prefix=composite_sr
echo "Enter aoi list"
read aoi_csv

conda activate composite
python $composite_exe --aoi_csv_pth=$aoi_csv --threads_number=$threads_num --config_filename=$config_filenm
mail -s "Composition Finished!" sye@clarku.edu <<< 'The composition task for $aoi_csv finished!'
