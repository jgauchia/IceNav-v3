# IceNav-v3
ESP32 GPS Navigator 

       Pinout CUSTOMBOARD:
       HCM5883L      BME280        MPU6050       ILI9488        SD CARD        VBAT             GPS
       -----------------------------------------------------------------------------------------------------
       VCC 3,3v      VCC 5v        VCC 3.3v      VCC  3,3v      VCC  3,3v      GPIO34           VCC  3,3v
       GND GND       GND GND       GND GND       GND  GND       GND  GND       ADC1_CHANNEL_6   GND  GND
       SDA GPIO21    SDA GPIO21    SDA GPIO21    LED  GPIO33    CS   GPIO4                      RX   GPIO25
       SCL GPIO22    SCL GPIO22    SCL GPIO22    MISO GPIO27    MISO GPIO19                     TX   GPIO26
                                                 SCK  GPIO14    SCK  GPIO17
                                                 MOSI GPIO13    MOSI GPIO23
                                                 DC   GPIO15
                                                 RST  GPIO32
                                                 CS   GPIO2
                                                 LED  GPIO33
                                                 TCH  GPIO18


       Pinout TDISPLAY:
       HCM5883L      ST7789         SD CARD        VBAT             GPS
       ------------------------------------------------------------------------
       VCC 3,3v      LED  GPIO4     VCC  3,3v      GPIO34           VCC  3,3v
       GND GND       SCK  GPIO18    GND  GND       ADC1_CHANNEL_6   GND  GND
       SDA GPIO21    MOSI GPIO19    CS   GPIO2                      RX   GPIO25
       SCL GPIO22    DC   GPIO16    MISO GPIO27                     TX   GPIO26
                     RST  GPIO23    SCK  GPIO13
                     CS   GPIO5     MOSI GPIO15
                                      

      TODO:
         * Language dependent texts
         * LVGL - optimization

SD Map Tile File structure

On SD Card map tiles (256x256 PNG Format) are stored in these folders structure:

      [ MAPS ]
         |________ [ zoom folder (number)]
                              |__________________ [ tile X folder (number)]
                                                             |_______________________ tile y file.png

      [UPDATE 09.12.2022]                                                             
         * Switch to LovyanGfx instead of TFT_eSPI 
         * Delete PNG Decoder (use LovyanGfx instead)
         * Delete BMP Decoder (use LovyanGfx instead)
         * Optimize events
         * Optimize draw map
         * Optimize LVGL tick (custom Tick in lv_conf)
         * Clean compile warnings
         * Fix Compass image
         * Change MAPS directory name
         * New Boot Logo and Splash Screen
         * CUSTOMBOARD:
               * Switch ILI9341 to ILI9488 
               * GPS serial GPIO changed (same as TDISPLAY)
               * Add Touch Support (Fix calibration)
               * Delete PCF8574 and external buttons
               * Fix Compass
               * Fix tile view
               * Fix Battery voltage
               * Added MPU6050
               * Fix touch update response
               * Fix LVGL redraw update
               * Disable widgets not used
               * Fix Serial Monitor with auto-upload wiring

      [UPDATE 12.12.2022]
         * Change Zoom box to slider

      [UPDATE 19.12.2022]
         * Store fixed texts in Flash (NOT RAM)
         * Add (again) LVGL Filesystem SD port
         * Fix LVGL display flush callback
         * Fix OSM Map position on screen
         * Add zoom label
         * Change GPS Reading to core 1
         * CUSTOBOARD:
               * Change SD SPI to VSPI 
               * Change LovyanGFX screen config
         
      [UPDATE 05.02.2023]
         * Upload port fixed to /dev/ttyUSB0
         * Fix Adafruit Sensor Libraries
         * Updated LovyanGFX Library
         * CUSTOMBOARD:
               * Added BME280 temp/press/hum. sensor
      
      [UPDATE 16.02.2023]
         * Number of satellites coloured is GPS signal is fixed

      [UPDATE 17.02.2023]
         * Fix Battery Reading
         * Fix GPS pinout in README.md

      [UPDATE 19.02.2023]   
         * Fix GPS satellites number (fix signal)
         * Add Charge Icon

      [UPDATE 22.02.2023]
         * Clean Code
         * Update Splash Screen (MCU Info)
         * Fix Battery Reading delay
         * Change Satellite Tracking screen (SNR Bars)
         * Add Event for Satellite Tracking
         * Fix Main Screen Update event
         * Add Event for Notify Bar

      [UPDATE 23.02.2023]         
         * Fix Some Compiling Warnings

      

