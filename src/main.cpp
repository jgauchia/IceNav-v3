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
#include <esp_wifi.h>
#include <esp_bt.h>

unsigned long millis_actual = 0;

#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/tft.h"
#include "hardware/keys.h"
#include "hardware/compass.h"
#include "hardware/battery.h"
#include "hardware/gps.h"
#include "hardware/power.h"
#include "utils/math.h"
#include "utils/bmp.h"
#include "utils/png.h"
#include "utils/wpt.h"
#include "gui/lvgl.h"
#include "gui/screens/splash_scr.h"
#include "tasks.h"

/**
 * @brief Setup
 *
 */
void setup()
{
#ifdef ENABLE_PCF8574
  keyboard.begin();
#endif

#ifdef ENABLE_COMPASS
  compass.begin();
#endif

#ifdef DEBUG
  init_serial();
#endif
  powerOn();
  init_sd();
  init_LVGL();
  init_tft();
  init_gps();

  batt_level = battery_read();

  millis_actual = millis();
  splash_scr();
  while (millis() < millis_actual + 4000)
    ;

  init_tasks();

  // lv_scr_load(searchSat);

  // lv_scr_load(mainScreen);
  // create_notify_bar();

  setPngPosition(0, 50);
  load_file(SD, "/MAP/17/66147/48885.png");
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  // xSemaphoreTake(xSemaphore, portMAX_DELAY);
  // lv_task_handler();
  // xSemaphoreGive(xSemaphore);
  // delay(5);
}
