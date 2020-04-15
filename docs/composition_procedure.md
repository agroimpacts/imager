# Composition procedure
### Author: Su Ye

These notes describe the steps for running planet image composition for productions

The steps are as following:
Pre: download github imager repo
#### Step 1: create instances 
1) Run script launch_ami_production.sh in github repo ''./imager/tool/create_ami_production.sh' (if you decide to use the old image, please skip this step)
2) Create spot-based instance based on ami via editing /imager/scripts/launch_ami_production.sh:
a. Change ‘INAME’ (line 5) each time into the ID of the instance you gonna to create. For example, if it is the third instance you would create, please suggested to give a name as “planet_compositer_spot3”
b. Update AMIID (line 1) to the new ID, and make sure that it is alighed with amiid of composition_V2 (if you decide to use the old image, please skip this step)
c. Specify the spot life cycle: change ‘ValidUntil’ in Line 9 based your need
3) Run launch_ami_production.Run this script locally to launch a spot-based instance: 
    ```bash
    ./launch_ami_productuion.sh
    ```
#### Step 2: run composition python script
1) Pre-work 
Log into the composition instance you just launched 
    ```bash
    ssh ubuntu@ec2-**-**-**-***.compute-1.amazonaws.com
    cd /home/ubuntu/imager/scripts
    ```
    
    Note: For aoi 10, 11, 13, 14, 16 in ghana, you need to change the window of defined season, please create a new yaml and upload to s3 first, and then modify bash script ‘run_composition_aoi.sh,’: for example, if you want to increase the window of dry season into four month, make cvmapper_config_composite_4monthdry.yaml and upload it to S3://activemapper, and then change yaml name in the bash script by modifying ‘config_filenm=cvmapper_config_composite.yaml’ to ‘config_filenm=cvmapper_config_composite_4monthdry.yaml’ in  /home/ubuntu/imager/scripts/run_composition_aoi.sh, then run.

2) Run composition using screen 
    ```bash
    Screen
    cd /home/ubuntu/imager/scripts
    ```
    Please change the email to your address in run_segmentation.sh, and then run it using:
    ```bash
    bash -i run_composition_aoi.sh
    ```
    Input the aoi name, and then 'ctrl + A + D' to return to main screen. You can feel free to log out the instance 
5. Once it finished, the instance would send a notisficiation email to your email address. If you didn't receive it, please check your junk folder also, and the notisification email is likely to be misidentified as junk email by your email app.
    You need to cancel spot-based instance after it is finished

3) (optional) In case for a new region:
   a. change in S3://activemapper//cvmapper_config_composite_congo.yaml, and then rename and upload it to S3://activemapper
    output_prefix: composite_sr_congo
    tile_geojson_path: new_tile_geojson_path
    img_catalog_name: new_img_catalog_name
    img_catalog_pth: new_img_catalog_pth
    dry_lower_ordinal: new_dry_lower_ordinal
    dry_upper_ordinal: new_dry_upper_ordinal  
    wet_lower_ordinal: new_wet_lower_ordinal
    wet_upper_ordinal: new_wet_upper_ordinal
   b. Once you started the instance, change 'config_filenm' in run_composition_aoi.sh into the new yaml name you gave in the above step, and then you should be ready to go


#### Step3: run make-up test
1) detected the composites that are missing from step2 by modifying missingtile_detector.R in the git repo: /imager/scripts. Upload csv to the folder path: /home/ubuntu/source
2) log into an active instance
   ```bash
    cd /home/ubuntu/imager/python
    screen
    conda activate composite
    python CompositionMaker.py --csv_pth=/home/ubuntu/source/missingtiles_0902019.csv --threads_number=4
    ```
    cltl+a+d
    Note: you may need to split missingtiles into two files,  3 month and 4 month, and run them separately using different procedures

#### Putting to glacial:
S3 bucket -> Activemapper -> Management -> Lifecycle -> Add lifecycle rule

#### Restore from glacial
1) generate text file which lists the locations of files needs to restore
Notes: we need to send a large number of commands, so better split the list into several texts for sub-lists (see ‘glacial_restoration.R’)
2) setup an instance (or multiple instance for splitted texts), and vi batch_restore.sh, and change line 11: done < url_planet_4bands_tie4.txt to one < textnameyouwanttoinput.txt
3) check the restoration status
aws s3api head-object --bucket activemapper --key planet/analytic_sr_all/003193cc-a8c2-4651-9d61-cc17c8907491/1/files/PSScene4Band/20181128_101218_1006/analytic_sr/20181128_101218_1006_3B_AnalyticMS_SR.tif
Note: replace the directory of file into the last directory given in your text file

#### Running single tile composition for test
   ```bash
    cd imager/python
    conda activate composite
    python CompositionMaker.py --config_filename=cvmapper_config_composite_congo.yaml --tile_id=680941 --bsave_ard=False
    ``` 
  
