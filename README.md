# IceNav-v3
ESP32 GPS Navigator 

       Pinout:
       HCM5883L      ILI9341        MICRO SD       VBAT        GPS
       --------------------------------------------------------------------
       VCC 3,3v      VCC  3,3v      VCC  3,3v      GPIO34      VCC  3,3v
       GND GND       GND  GND       GND  GND                   GND  GND
       SDA GPIO21    LED  GPIO33    CS   GPIO4                 RX   GPIO17
       SCL GPIO22    MISO GPIO27    MISO GPIO27                TX   GPIO16
                     SCK  GPIO14    SCK  GPIO14
                     MOSI GPIO13    MOSI GPIO13
                     DC   GPIO15
                     RST  GPIO32
                     CS   GPIO2


       Pinout TDISPLAY:
       HCM5883L      ST7789         MICRO SD       VBAT        GPS
       --------------------------------------------------------------------
       VCC 3,3v      LED  GPIO4     VCC  3,3v      GPIO34      VCC  3,3v
       GND GND       SCK  GPIO18    GND  GND                   GND  GND
       SDA GPIO21    MOSI GPIO19    CS   GPIO2                 RX   GPIO26
       SCL GPIO22    DC   GPIO16    MISO GPIO27                TX   GPIO25
                     RST  GPIO23    SCK  GPIO15
                     CS   GPIO5     MOSI GPIO13
                     
                     
       Librerías:
       ILI9341 :  https://github.com/Bodmer/TFT_eSPI
       GPS:       https://github.com/mikalhart/TinyGPSPlus
       PCF8574:   https://github.com/RobTillaart/PCF8574
       Batería:   https://github.com/danilopinotti/Battery18650Stats
       MyDelay:   https://github.com/mggates39/MyDelay


       Archivos necesarios para leer PNG:

       include
          |__________ miniz.c
          |__________ miniz.h
          |__________ support_functions.h

       pngle.c
       pngle.h

      For Using TTGO T-Display uncomment (config.h)
        #define TDISPLAY
   