#!/bin/bash
composite_exe=/home/ubuntu/imager/python/CompositionMaker.py
config_filenm=cvmapper_config_composite_congo.yaml
threads_num=4
echo "Enter aoi"
read aoi_id

conda activate composite
python $composite_exe --aoi=$aoi_id --threads_number=$threads_num --config_filename=$config_filenm
mail -s "Composition Finished!" <emailaddress> <<< 'The composition task for finished: aoi'$aoi_id
