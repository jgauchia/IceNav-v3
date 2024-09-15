## SD Map Tile File structure

Using [Maperitive](http://maperitive.net/) select your zone and generate your tiles. For that enter to `MAP-> Set Geometry bounds` draw or expand the square of your zone and run the command `generate-tiles minzoom=6 maxzoom=17`, It could takes long time, maybe 1 hour or more depending your area.

![Maperitive zone selection](images/maperitive_zone_selection.jpg)

After that, copy the contents of directory `Tiles` into your SD in a directory called `MAP`.

On SD Card map tiles (256x256 PNG Format) should be stored, in these folders structure:

      [ MAP ]
         |________ [ zoom folder (number) ]
                              |__________________ [ tile X folder (number) ]
                                                             |_______________________ tile Y file.png

## SD Vectorized Map File structure          

Using [OSM_Extract](https://github.com/aresta/OSM_Extract) you can generate binary map files to later create vector maps. Once generated, these files should be saved in the `mymap` folder on the SD card.

The PBF files can be downloaded from the [geofabrik](https://download.geofabrik.de/) website.

The PBF files should be saved in the `pbf` directory. Once saved, you should select the region or boundaries for which the GeoJSON files will be generated.

To obtain the boundaries, please check the [geojson.io](http://geojson.io) website.

For generate GeoJSON files run inside `maps` directory:

```bash
ogr2ogr -t_srs EPSG:3857 -spat min_lon min_lat max_lon max_lat map_lines.geojson /pbf/downloaded.pbf lines

ogr2ogr -t_srs EPSG:3857 -spat min_lon min_lat max_lon max_lat map_polygons.geojson /pbf/downloaded.pbf multipolygons
```

For generate binary map files run inside `maps` directory.
```bash
/scripts/./extract_features.py min_lon min_lat max_lon max_lat map
```
Once the process is completed, the maps will be inside the `maps/mymap` directory. Copy all folders to the SD card except the `test_imgs` directory.

Please follow the instructions provided by [OSM_Extract](https://github.com/aresta/OSM_Extract) for any further questions.