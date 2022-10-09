/*
       @file       main.cpp
       @brief      Programa receptor y navegador GPS con ESP32 + GPS + ILI9341 + HCM5883L

       @author     Jordi Gauchia

       @date       08/12/2021

       Pinout & Info view README.md:
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TinyGPS++.h>
//#include <FS.h>
//#include <SD.h>
#include <TimeLib.h>
#include <PCF8574.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

#include <Battery18650Stats.h>
#include <WiFi.h>
#include <MyDelay.h>
#include <esp_wifi.h>
#include <esp_bt.h>

// new
#include "config.h"
#include "gui/icons.h"
#include "hal.h"
#include "serial.h"
#include "sdcard.h"


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

void setup()
{
#ifdef DEBUG
  init_serial();
#endif
  init_ili9341();
  init_sd();
  init_gps();
  init_icenav();
  init_tasks();
}

void loop()
{
}
