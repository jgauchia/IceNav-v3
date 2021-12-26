/*
       @file       IceNav3.ino
       @brief      Programa receptor y navegador GPS con ESP32 + GPS + ILI9341 + HCM5883L

       @author     Jordi Gauchia

       @date       08/12/2021

       Pinout:
       HCM5883L      ILI9341        MICRO SD       VBAT        GPS
       --------------------------------------------------------------------
       VCC 3,3v      VCC  3,3v      VCC  3,3v      GPIO34      VCC  3,3v
       GND GND       GND  GND       GND  GND                   GND  GND
       SDA GPIO21    LED  GPIO33    CS   GPIO04                RX   GPIO17
       SCL GPIO22    MISO GPIO27    MISO GPIO27                TX   GPIO16
                     SCK  GPIO14    SCK  GPIO14
                     MOSI GPIO13    MOSI GPIO13
                     DC   GPIO15
                     RST  GPIO32
                     CS   GPIO2

       Librerías:
       ILI9341 :  https://github.com/Bodmer/TFT_eSPI
       GPS:       https://github.com/mikalhart/TinyGPSPlus
       PCF8574:   https://github.com/RobTillaart/PCF8574
       Batería:   https://github.com/danilopinotti/Battery18650Stats

       Archivos necesarios para leer PNG:
       miniz.c / miniz.h
       pngle.c / pingle.h
       suport_functions.h
*/

#define DEBUG 1
//#define OUTPUT_NMEA 1
//#define SEARCH_SAT_ON_INIT 1

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <FS.h>
#include <SD.h>
#include <TimeLib.h>
#include <PCF8574.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Battery18650Stats.h>
#include "Z_Bitmaps.h"
#include "0_Vars.h"
#include "1_Func.h"
#include "2_Func_BMP.h"
#include "3_Func_GPS.h"
#include "4_Func_GFX.h"
#include "5_Func_Math.h"
#include "6_Func_Keys.h"
#include "7_Func_Bruj.h"
#include "8_Func_Batt.h"
#include "A_Pantallas.h"
#include "ZZ_Core_Funcs.h"
#include "support_functions.h"

void setup()
{
  init_serial();
  init_ili9341();
  init_sd();
  init_gps();
  init_icenav();
  init_tasks();
}

void loop()
{
}
