/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1
 * @date 2022-10-09
 */

#include <Arduino.h>
#include <Wire.h>
#include <Ticker.h>
#include <SPI.h>
#include <WiFi.h>
#include <MyDelay.h>
#include <esp_wifi.h>
#include <esp_bt.h>

unsigned long millis_actual = 0;

#include "hardware/tft.h"
#include "gui/lvgl.h"
#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/compass.h"
#include "hardware/battery.h"
#include "hardware/gps.h"
#include "hardware/keys.h"
#include "hardware/power.h"
#include "utils/math.h"
#include "utils/bmp.h"
#include "utils/png.h"
#include "utils/wpt.h"

#include "gui/screens/splash_scr.h"
//#include "gui/icons.h"
//#include "gui/screens.h"
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
  powerOn();
  init_LVGL();
  init_tft();
  init_sd();
  init_gps();

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

  init_tasks();

  lv_scr_load(searchSat);
  get_gps_fix();

  lv_obj_t *tv = lv_tileview_create(NULL);
  lv_obj_t *tile1 = lv_tileview_add_tile(tv, 0, 0, LV_DIR_LEFT);
  lv_obj_t *tile2 = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
  lv_obj_t *tile3 = lv_tileview_add_tile(tv, 2, 0, LV_DIR_LEFT);

  lv_scr_load(tv);

  // lv_obj_t *currentScreen = lv_scr_act();
}

/**
 * @brief Main Loop
 *
 */
void loop()
{

  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  lv_task_handler();
  xSemaphoreGive(xSemaphore);
  delay(5);
}
