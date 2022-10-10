/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1
 * @date 2022-10-09
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <MyDelay.h>
#include <esp_wifi.h>
#include <esp_bt.h>

// Old - TO-DO -> REMOVE THESE INCLUDES
#include "0_Vars.h"

// New - TO-DO -> NEW INCLUDES MIGRATED
#include "config.h"
#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/compass.h"
#include "hardware/battery.h"
#include "hardware/gps.h"
#include "hardware/tft.h"
#include "utils/math.h"
#include "utils/bmp.h"
#include "gui/icons.h"
#include "gui/screens.h"
#include "hardware/keys.h"

// Old - TO-DO -> REMOVE THESE INCLUDES
void load_file(fs::FS &fs, const char *path);
#include "pngle.h"
#include "support_functions.h"
void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]);
#include "4_Func_GFX.h"
#include "A_Pantallas.h"


#include "tasks.h"

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

#ifdef DISABLE_RADIO
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  esp_wifi_stop();
  esp_bt_controller_disable();
#endif

#ifdef ENABLE_PCF8574
  keyboard.begin();
  KEYStime.start();
#endif

#ifdef ENABLE_COMPASS
  compass.begin();
  COMPASStime.start();
#endif

  BATTtime.start();
  batt_level = Read_Battery();

  millis_actual = millis();
  splash_scr();
  while (millis() < millis_actual + 4000)
    ;
  tft.fillScreen(TFT_BLACK);
#ifdef SEARCH_SAT_ON_INIT
  search_sat_scr();
#endif

  is_menu_screen = false;
  is_main_screen = true;

  init_tasks();
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
}
