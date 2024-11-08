![Static Badge](https://img.shields.io/badge/PlatformIO-PlatformIO?logo=platformio&labelColor=auto&color=white)
[![PlatformIO](https://github.com/jgauchia/IceNav-v3/workflows/PlatformIO/badge.svg)](https://github.com/jgauchia/IceNav-v3/actions/) ![ViewCount](https://views.whatilearened.today/views/github/jgauchia/IceNav-v3.svg)

![icenav_logo](images/icenav_logo.png)

ESP32 Based GPS Navigator (LVGL - LovyanGFX).
* Note: Under development (experimental features under devel branch)
* There is the possibility to use two types of maps: Rendered Maps or Tiles (large files), and Vector Maps (small files).

<table>
  <tr>
    <td>
      Don't forget to star ⭐️ this repository
    </td>
   <a href="https://www.buymeacoffee.com/jgauchia" target="_blank" title="buymeacoffee"><img src="https://iili.io/JIYMmUN.gif"  alt="buymeacoffee-animated-badge" style="width: 160px;"></a>
  </tr>
</table>

> [!CAUTION]
> Do not use in production (Experimental features).

## Screenshots
<details><summary>Click me</summary>
  
|<img src="images/dev/splash.jpg">|<img src="images/dev/searchsat.jpg">|<img src="images/dev/compass.jpg">|<img src="images/dev/options.jpg">|<img src="images/dev/wptopt.jpg">|
|:-:|:-:|:-:|:-:|:-:|
| Splash Screen | Search Satellite | Compass | Main Options | Waypoint Options |

|<img src="images/dev/rendermap.jpg">|<img src="images/dev/vectormap.jpg">|<img src="images/dev/navscreen.jpg">|<img src="images/dev/navscreen2.jpg">|<img src="images/dev/satelliteinfo.jpg">|
|:-:|:-:|:-:|:-:|:-:|
| Rendered Map | Vectorized Map | Navigation Screen | Navigation Screen | Satellite Info |

|<img src="images/dev/addwpt_n.jpg">|<img src="images/dev/addwpt_l.jpg">|<img src="images/dev/wptlist.jpg">|
|:-:|:-:|:-:|
| Add Waypoint | Add Waypoint (landscape) | Waypoint List |

|<img src="images/dev/settings.jpg">|<img src="images/dev/compasscal.jpg">|<img src="images/dev/touchcal.jpg">|<img src="images/dev/mapsettings.jpg">|<img src="images/dev/devicesettings.jpg">|
|:-:|:-:|:-:|:-:|:-:|
| Settings | Compass Calibration | Touch Calibration | Map Settings | Device Settings |

### WiFi CLI Manager
![WifiCLI](https://github.com/jgauchia/IceNav-v3/assets/1075178/a7f8af18-2c34-436d-8fef-995540312cb2)

### Web File Server 
![webfile](https://github.com/user-attachments/assets/ce38f3b6-d8ab-4540-8d01-a2b393cc5898)

</details>

## Specifications

Currently, IceNav works with the following hardware setups and specs

**Highly recommended an ESP32S3 with PSRAM and 320x480 Screen** 
 
> [!IMPORTANT]
> Please review the platformio.ini file to choose the appropriate environment as well as the different build flags for your correct setup.

### Boards

|                        | FLASH | PSRAM | Environment                  | Full Support |
|:-----------------------|:-----:|:-----:|:-----------------------------|--------------|
| ICENAV (ESP32S3)       |  16M  |  8M   | ``` [env:ICENAV] ```         |     YES      |
| ESP32                  |  16M  |  4M   | ``` [env:ESP32_N16R4] ```    |     YES      |
| ESP32S3                |  16M  |  8M   | ``` [env:ESP32S3_N16R8] ```  |     YES      |
| [ELECROW ESP32 Terminal](https://www.elecrow.com/esp-terminal-with-esp32-3-5-inch-parallel-480x320-tft-capacitive-touch-display-rgb-by-chip-ili9488.html) |  16M  |  8M   | ``` [env:ELECROW_ESP32] ```  | YES [^1] [^2]|
| [MAKERFABS ESP32S3](https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-ili9488.html) |  16M  |  2M   | ``` [env:MAKERF_ESP32S3] ``` |   TESTING    |
| [LILYGO T-DECK](https://www.lilygo.cc/products/t-deck) |  16M  |  8M   | ``` [env:TDECK_ESP32S3] ``` |   YES    |

If the board has a BOOT button (GPIO0) it is possible to use power saving functions.
To do this, simply include the following Build Flag in the required env in platformio.ini

```-DPOWER_SAVE```
```-DARDUINO_RUNNING_CORE=1```
```-DARDUINO_EVENT_RUNNING_CORE=1```

> [!IMPORTANT]
> Currently, this project can run on any board with an ESP32S3 and at least a 320x480 TFT screen. The idea is to support all existing boards on the market that I can get to work, so if you don't want to use the specific IceNav board, please feel free to create an issue, and I will look into providing support.
> Any help or contribution is always welcome

### Screens

| Driver [^2] | Resolution | SPI | 8bit | 16bit | Touch     | Build Flags [^3]                 |
|:------------|:----------:|:---:|:----:|:-----:|:---------:|:---------------------------------|
| ILI9488 [^4]| 320x480    | yes | ---  | ---   | XPT2046   | ```-DILI9488_XPT2046_SPI```      |
| ILI9488     | 320x480    | yes | ---  | ---   | FT5x06    | ```-DILI9488_FT5x06_SPI```       |
| ILI9488     | 320x480    | --- | yes  | ---   | --------  | ```-DILI9488_NOTOUCH_8B```       |
| ILI9488     | 320x480    | --- | ---  | yes   | FT5x06    | ```-DILI9488_FT5x06_16B```       |
| ILI9341     | 320x240    | yes | ---  | ---   | XPT2046   | ```-DILI9341_XPT2046_SPI```      |

If TFT shares SPI bus with SD card add the followings Build Flag to platformio.ini

```-DSPI_SHARED```

### Modules

|             | Type          | Build Flags [^3]                 | lib_deps [^5] (**no common environment**)              |
|:------------|:--------------|:---------------------------------|:-------------------------------------------------------|
|             | Batt. Monitor | ```-DADC1``` or ```-DADC2``` <br> ```-DBATT_PIN=ADCn_CHANNEL_x``` |                       |   
| AT6558D     | GPS           | ```-DAT6558D_GPS```              |                                                        |
| HMC5883L    | Compass       | ```-DHMC5883L```                 | ```dfrobot/DFRobot_QMC5883@^1.0.0```                   |
| QMC5883     | Compass       | ```-DQMC5883```                  | ```dfrobot/DFRobot_QMC5883@^1.0.0```                   |
| MPU9250     | IMU (Compass) | ```-DIMU_MPU9250```              | ```bolderflight/Bolder Flight Systems MPU9250@^1.0.2```|
| BME280      | Temp/Pres/Hum | ```-DBME280```                   | ```adafruit/Adafruit Unified Sensor@^1.1.14``` <br> ```adafruit/Adafruit BusIO@^1.16.1``` <br> ```adafruit/Adafruit BME280 Library@^2.2.4```|

[^1]: For ELECROW board UART port is shared with USB connection, GPS pinout are mapped to IO19 and IO40 (Analog and Digital Port). If CLI isn't used is possible to attach GPS module to UART port but for upload the firmware (change pinout at **hal.hpp**), the module should be disconnected.
[^2]: See **hal.hpp** for pinouts configuration
[^3]: **platformio.ini** file under the build_flags section
[^4]: If Touch SPI is wired to the same SPI of ILI9488 ensure that TFT MISO line has 3-STATE for screenshots (read GRAM) or leave out 
[^5]: You need to add libraries dependencies if the buid flag requires

Other setups like another sensors types, etc... not listed in the specs, now **They are not included**

If you wish to add any other type of sensor, module, etc., you can create a PR without any problem, and we will try to implement it. Thank you!
</details>

## Wiring

See **hal.hpp** for pinouts configuration

## SD Renderized Map Tile File structure

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

## Firmware install


> [!IMPORTANT]
>Please install first [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.
> 
>For ESP32 board run:
> 
>```bash
>pio run --target upload
>```
>
>For ESP32S3 Makerfab board:
> 
>```bash
>pio run -e MAKERF_ESP32S3 --target upload
>```
>
> For Other boards:
>
> ```bash
> pio run -e environment --target upload
> ```
> 
> After this, load the icons and assets with:
> 
> ```bash
> pio run --target uploadfs
> ```


> [!TIP]
> Optional, for map debugging with specific coordinates, or when you are in indoors, you are able to set the defaults coordinates, on two ways:

> **Using the CLI**
>
> Using the next commands to set your default coordinates, for instance:
>
> ```bash
> klist
> kset defLAT 52.5200
> kset defLON 13.4049
> ```
> 
> **Using enviroment variables**:
>
> Export your coordinates before to build and upload, for instance:
> 
> ```bash
> export ICENAV3_LAT=52.5200
> export ICENAV3_LON=13.4049
> pio run --target upload
> ```

## CLI

IceNav has a basic CLI accessible via Serial and optionally via Telnet if enabled. When you access the CLI and type `help`, you should see the following commands:

```bash
clear:          clear shell
info:           get device information
klist:          list of user preferences. ('all' param show all)
kset:           set an user extra preference
nmcli:          network manager CLI. Type nmcli help for more info
outnmea:        toggle GPS NMEA output (or Ctrl+C to stop)
poweroff:       perform a ESP32 deep sleep
reboot:         perform a ESP32 reboot
scshot:         screenshot to SD or sending a PC
settings:       device settings
waypoint:       waypoint utilities
webfile:        enable/disable Web file server
wipe:           wipe preferences to factory default
```

Some extra details:

**nmcli**: IceNav use a `wcli` network manager library. For more details of this command and its sub commands please refer to [here](https://github.com/hpsaturn/esp32-wifi-cli?tab=readme-ov-file#readme)

**outnmea**: this command toggle the GPS output to the serial console. With that it will be compatible with external GPS software like `PyGPSClient` and others. To stop these messages in your console, just only repeat the same command or perform a `CTRL+C`.

**scshot**: This utility can save a screenshot to the root of your SD, with the name: `screenshot.raw`. You can convert it to png using the `convert.py` script in the `tools` folder.

Additionally, this screenshot command can send the screenshot over WiFi using the following syntax (replace IP with your PC IP):

```bash
scshot 192.168.1.10 8123
```

Ensure your PC has the specified port open and firewall access enabled to receive the screenshot via the `netcat` command, like this:

```bash
nc -l -p 8123 > screenshot.raw
```

**settings**: Device settings type `settings` for detailed options.

**waypoint**: type `waypoint` for detailed options.

Additionally, this waypoint command can send the waypoint over WiFi using the following syntax (replace IP with your PC IP):

```bash
waypoint down file.gpx 192.168.1.10 8123
```

Ensure your PC has the specified port open and firewall access enabled to receive the waypoint file via the `netcat` command, like this:

```bash
nc -l -p 8123 > waypoint.gpx
```

## Web File Server 

IceNav has a small web file server (https://youtu.be/IYLcdP40cU4) to manage existing files on the SD card.
An active WiFi connection is required (to do this, see how to do it using CLI).

The Web File Server will start automatically if default automatic network connection is enabled (see CLI).

To access the Web File Server, simply use any browser and go to the following address: http://icenav.local

> [!CAUTION]
> This feature has known issues (failures when uploading/downloading some large files, web refresh...) if the SPI bus is shared between the TFT and the SD card. Tests with boards that do not share this bus have been successful.
> Work is underway to fix this so that it can be applied to the final design of the IceNav board.



## TO DO

- [X] LVGL 9 Integration
- [X] Support other resolutions and TFT models
- [ ] Support for ready-made boards 
- [X] Wifi CLI Manager
- [X] LVGL Optimization 
- [ ] GPX Integration
- [ ] Multiple IMU's and Compass module implementation
- [X] Power saving
- [X] Vector maps
- [ ] Google Maps navigation style
- [x] Optimize code
- [ ] Fix bugs!
- [X] Web file server
      

## Special thanks to....
* [@hpsaturn](https://github.com/hpsaturn) Thanks to him and his knowledge, this project is no longer sitting in a drawer :smirk:.
* [@Elecrow-RD](https://github.com/Elecrow-RD)  For your interest in my project and for providing me with hardware to test it.
* [@pcbway](https://github.com/pcbway) for bringing a first prototype of the IceNav PCB to reality :muscle:
* [@lovyan03](https://github.com/lovyan03/LovyanGFX) for his library; I still have a lot to learn from it.
* [@lvgl](https://github.com/lvgl/lvgl) for creating an amazing UI
* And of course, to my family, who supports me through all this development and doesn’t understand why. :kissing_heart: I will never be able to thank you enough for the time I've dedicated.


## Credits

* Added support to [Makerfabs ESP32-S3 Parallel TFT with Touch 3.5" ILI9488](https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-ili9488.html) from [@makerfabs](https://github.com/makerfabs) thanks to [@hpsaturn](https://github.com/hpsaturn) to test it!
* Improved documentation thanks to [@hpsaturn](https://github.com/hpsaturn)
* Improved auto mainScreen selection from env variable preset thanks to [@hpsaturn](https://github.com/hpsaturn)
* Improved getLat getLon from environment variables thanks to [@hpsaturn](https://github.com/hpsaturn)
* 3DPrint case for an ESP32S3 Makerfabs Parallel board thanks to [@hpsaturn](https://github.com/hpsaturn)
* Vectorial Maps routines [ESP32_GPS](https://github.com/aresta/ESP32_GPS) thanks to [@aresta](https://github.com/aresta)
* OSM to binary vectorial maps [OSM_Extract](https://github.com/aresta/OSM_Extract) thanks to [@aresta](https://github.com/aresta)
* Preferences Library [Easy Preferences](https://github.com/hpsaturn/easy-preferences) thanks to [@hpsaturn](https://github.com/hpsaturn)
* Wifi CLI manager [esp32-wifi-cli](https://github.com/hpsaturn/esp32-wifi-cli) thanks to [@hpsaturn](https://github.com/hpsaturn)
* Web file server based in [@smford](https://github.com/smford) [esp32-asyncwebserver-fileupload-example ](https://github.com/smford/esp32-asyncwebserver-fileupload-example)


---
Map data is available thanks to the great OpenStreetMap project and contributors. The map data is available under the Open Database License.

© OpenStreetMap contributors
