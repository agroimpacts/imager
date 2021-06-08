imager
================

This is the repo for `imager`, the image downloading and compositing
component of the active learning cropland mapping system.

## Working with AWS resources

  - [Starting and configuring a jupyter notebook capable `imager`
    instance](docs/start-configure-imager.md)
  - [Creating an instance template and launching new
    instances](docs/create-ami-new-instance.md)

## Imagery

  - Querying and downloading from the Planet API

All the scripts are in planet folder.

**First** part is delivering Planetscope images to S3 using ‘porder’
package.

Simply run the `planet_downloader.R` to deliver the images. But
sometimes there might be problem, you also can run the functions one by
one to make sure everything is fine. The functions are in
`downloader_functions.R`. To understand better about the functions, read
the `porder` document first.

**Second** part is registering the images to raster foundry and update
the scenes\_data table in database.

  - Function `aligh_grid_tile.R` is to extract master grid id for a
    given tile geojson file, e.g. ghana\_tiles.geojson.

  - Function `generate_catalog_before_rf.R` is to make the catalog file
    for cvml and save it to S3.

  - Function `fill_scenes_data.R` is to generate the tms\_url for the
    images saved by `generate_catalog_before_rf.R` and update the
    scenes\_data table in database.

  - Function `rasterfoundry_register_each.py` is a python script to
    generate the tms\_url for one scene, e.g. `python3
    rasterfoundry_register_each.py --scene_id *** --url ***`

  - Function `rasterfoundry_register_csv.py` is a python script to
    update a csv catalog table, hence the csv table must have cell\_id,
    scene\_id, col, row, season, url, tms\_url columns to make the
    script run properly.

The most important step to do this part is to set the `config.yaml` file
(in planet/cfg), then run `aligh_grid_tile.R`,
`generate_catalog_before_rf.R`, and `fill_scenes_data.R` one by one.

**Third** is creating cloud-free composite. Instructions for launching composition procedure can be found [here](docs/composition_procedure.md)

## Citation: \
Estes, L. D., Ye, S., Song, L., Luo, B., Eastman, J. R., Meng, Z., ... & Caylor, K. (2021). High resolution, annual maps of the characteristics of smallholder-dominated croplands at national scales (https://eartharxiv.org/repository/view/2155/)
