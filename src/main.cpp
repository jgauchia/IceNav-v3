/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1
 * @date 2022-10-09
 */

#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false

#include <Arduino.h>
#include <Wire.h>
#include <Ticker.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Timezone.h>

unsigned long millis_actual = 0;

#include "hardware/hal.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"
#include "hardware/tft.h"
#ifdef ENABLE_COMPASS
#include "hardware/compass.h"
#endif
#ifdef ENABLE_BME
#include "hardware/bme.h"
#endif
#include "hardware/battery.h"
#include "hardware/gps.h"
#include "hardware/power.h"
#include "utils/gps_math.h"
#include "utils/wpt.h"
#include "utils/lv_spiffs_fs.h"
#include "utils/lv_sd_fs.h"
#include "utils/time_zone.h"
#include "gui/lvgl.h"

#include "tasks.h"

/**
 * @brief Setup
 *
 */
void setup()
{

#ifdef ENABLE_BME
  bme.begin(BME_ADDRESS);
#endif
#ifdef ENABLE_COMPASS
  compass.begin();
#endif

#ifdef DEBUG
  init_serial();
#endif
  powerOn();
  init_sd();
  init_SPIFFS();
#ifdef MAKERF_ESP32S3
  Wire.end();
#endif
  init_LVGL();
  init_tft();
  init_gps();
  init_ADC();

  batt_level = battery_read();
  splash_scr();
  init_tasks();

  lv_scr_load(searchSat);
  // lv_scr_load(mainScreen);
  // create_notify_bar();
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  lv_timer_handler();
  delay(5);
}
