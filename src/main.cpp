/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1
 * @date 2022-10-09
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <MyDelay.h>
#include <esp_wifi.h>
#include <esp_bt.h>

// Old - TO-DO -> REMOVE THESE INCLUDES
#include <TFT_eSPI.h>
#include <SPI.h>
#include "0_Vars.h"

// New - TO-DO -> NEW INCLUDES MIGRATED
#include "config.h"
#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/compass.h"
#include "hardware/battery.h"
#include "hardware/keys.h"
#include "hardware/gps.h"
#include "utils/math.h"
#include "utils/bmp.h"
#include "gui/icons.h"

// Old - TO-DO -> REMOVE THESE INCLUDES
#include "1_Func.h"
#include "4_Func_GFX.h"
#include "5_Func_Math.h"
#include "A_Pantallas.h"
#include "ZZ_Core_Funcs.h"

// New - TO-DO -> NEW INCLUDES MIGRATED
#include "gui/screens/search_sat.h"

/**
 * @brief Setup
 * 
 */
void setup()
{
#ifdef DEBUG
  init_serial();
#endif
  init_tft();
  init_sd();
  init_gps();
  init_icenav();
  init_tasks();
}

/**
 * @brief Main Loop
 * 
 */
void loop()
{
}
