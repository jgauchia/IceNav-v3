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
// **********************************************
//  Declaración funciones
// **********************************************
//void setPngPosition(int16_t x, int16_t y);
void show_sat_hour(int x, int y, int font);
void show_sat_tracking();
void show_sat_track_screen();
void show_main_screen();
void show_sat_track_screen();
void show_map_screen();
//******************************************************

// New - TO-DO -> NEW INCLUDES MIGRATED
unsigned long millis_actual = 0;
#include "config.h"
#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/compass.h"
#include "hardware/battery.h"
#include "hardware/gps.h"
#include "hardware/tft.h"
#include "hardware/keys_def.h"
#include "utils/math.h"
#include "utils/bmp.h"
#include "utils/png.h"
#include "gui/icons.h"
#include "gui/screens.h"
#include "hardware/keys.h"

// Old - TO-DO -> REMOVE THESE INCLUDES
#include "A_Pantallas.h"

// New - TO-DO -> NEW INCLUDES MIGRATED
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
