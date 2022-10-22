# IceNav-v3
ESP32 GPS Navigator 

       Pinout:
       HCM5883L      ILI9341        SD CARD        VBAT             GPS
       ------------------------------------------------------------------------
       VCC 3,3v      VCC  3,3v      VCC  3,3v      GPIO34           VCC  3,3v
       GND GND       GND  GND       GND  GND       ADC1_CHANNEL_6   GND  GND
       SDA GPIO21    LED  GPIO33    CS   GPIO4                      RX   GPIO17
       SCL GPIO22    MISO GPIO27    MISO GPIO27                     TX   GPIO16
                     SCK  GPIO14    SCK  GPIO14
                     MOSI GPIO13    MOSI GPIO13
                     DC   GPIO15
                     RST  GPIO32
                     CS   GPIO2
                     LED  GPIO33


       Pinout TDISPLAY:
       HCM5883L      ST7789         SD CARD        VBAT             GPS
       ------------------------------------------------------------------------
       VCC 3,3v      LED  GPIO4     VCC  3,3v      GPIO34           VCC  3,3v
       GND GND       SCK  GPIO18    GND  GND       ADC1_CHANNEL_6   GND  GND
       SDA GPIO21    MOSI GPIO19    CS   GPIO2                      RX   GPIO26
       SCL GPIO22    DC   GPIO16    MISO GPIO27                     TX   GPIO25
                     RST  GPIO23    SCK  GPIO13
                     CS   GPIO5     MOSI GPIO15
                     
                     
       Libraries:
       ILI9341 :  https://github.com/Bodmer/TFT_eSPI
       GPS:       https://github.com/mikalhart/TinyGPSPlus
       PCF8574:   https://github.com/RobTillaart/PCF8574

      TODO:
         * Language dependent texts
         * LVGL - optimization

SD Map Tile File structure

On SD Card map tiles (PNG Format) are stored in these folders structure:

      [ MAP ]
         |________ [ zoom folder (number)]
                              |__________________ [ tile X folder (number)]
                                                             |_______________________ tile y file.png