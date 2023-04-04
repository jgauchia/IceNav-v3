# IceNav-v3
ESP32 GPS Navigator 

       Pinout (ESP32-WROVER)
       HCM5883L      BME280        MPU6050       ILI9488        SD CARD        VBAT             GPS
       -----------------------------------------------------------------------------------------------------
       VCC 3,3v      VCC 5v        VCC 3.3v      VCC  3,3v      VCC  3,3v      GPIO34           VCC  3,3v
       GND GND       GND GND       GND GND       GND  GND       GND  GND       ADC1_CHANNEL_6   GND  GND
       SDA GPIO21    SDA GPIO21    SDA GPIO21    LED  GPIO33    CS   GPIO4     (Resist. div)    RX   GPIO25
       SCL GPIO22    SCL GPIO22    SCL GPIO22    MISO GPIO27    MISO GPIO19                     TX   GPIO26
                                                 SCK  GPIO14    SCK  GPIO12
                                                 MOSI GPIO13    MOSI GPIO23
                                                 DC   GPIO15
                                                 RST  GPIO32
                                                 CS   GPIO2
                                                 LED  GPIO33
                                                 TCH  GPIO18
                                                 TIRQ GPIO5

SD Map Tile File structure

On SD Card map tiles (256x256 PNG Format) are stored in these folders structure:

      [ MAPS ]
         |________ [ zoom folder (number)]
                              |__________________ [ tile X folder (number)]
                                                             |_______________________ tile y file.png


Specifications:

   * ESP32 WROVER with 4Mb PSRAM / 16 Mb Flash
   * ILI9488 TFT (320x480)
   * SD/MicroSD reader
   * HCM5883L Magnetometer
   * BME280   Temperature / Humidity sensor
   * MPU6050  Accelerometer and Gyroscope IMU
   * HT1818Z3G5L GPS Module
   * LVGL UI + LovyanGFX
