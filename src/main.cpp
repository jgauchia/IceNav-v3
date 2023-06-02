/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1.4
 * @date 2023-05-23
 */

#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
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
#include "utils/gps_maps.h"
#include "utils/gps_math.h"
#include "utils/sat_info.h"
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

  splash_scr();
  //init_tasks();

#ifdef DEFAULT_LAT
  lv_scr_load(mainScreen);
  create_notify_bar();
#else
  lv_scr_load(searchSat);
#endif
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  vTaskDelay(10);
#ifdef MAKERF_ESP32S3
  lv_tick_inc(5);
#endif
  lv_timer_handler();
  while (gps->available() > 0)
  {
#ifdef OUTPUT_NMEA
    {
      debug->write(gps->read());
    }
#else
    GPS.encode(gps->read());
#endif
  }
}
