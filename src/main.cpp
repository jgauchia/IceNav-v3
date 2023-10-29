/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1.6
 * @date 2023-06-14
 */

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
#include "hardware/gps.h"
#include "utils/preferences.h"
#include "hardware/serial.h"
#include "hardware/sdcard.h"

#include "utils/maps/maps.h"
#include "utils/graphics/graphics.h"
#include "hardware/tft.h"

#ifdef ENABLE_COMPASS
#include "hardware/compass.h"
#endif
#ifdef ENABLE_BME
#include "hardware/bme.h"
#endif
#include "hardware/battery.h"
#include "hardware/power.h"
#include "utils/gps_maps.h"
#include "utils/gps_math.h"
#include "utils/sat_info.h"
#include "utils/lv_spiffs_fs.h"
#include "utils/lv_sd_fs.h"
#include "utils/time_zone.h"
#include "gui/lvgl.h"

#include "tasks.h"

MemBlocks memBlocks;
ViewPort viewPort;
#define DEG2RAD(a) ((a) / (180 / M_PI))
#define RAD2DEG(a) ((a) * (180 / M_PI))
#define EARTH_RADIUS 6378137
double lat2y(double lat) { return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS; }
double lon2x(double lon) { return DEG2RAD(lon) * EARTH_RADIUS; }

/**
 * @brief Setup
 *
 */
void setup()
{
#ifdef MAKERF_ESP32S3
  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin();
#endif

#ifdef ENABLE_BME
  bme.begin(BME_ADDRESS);
#endif

#ifdef ENABLE_COMPASS
  init_compass();
#endif

#ifdef DEBUG
  init_serial();
#endif
  powerOn();
  load_preferences();
  init_sd();
  init_SPIFFS();
  init_LVGL();
  init_tft();
  init_gps();
  init_ADC();

  map_spr.deleteSprite();
  map_spr.createSprite(768, 768);

  // splash_scr();

  // init_tasks();

  header_msg("Reading map...");

  // Point32 map_center(lon2x(2.01439), lat2y(41.57353));
   Point32 map_center( 225680.32, 5084950.61);
  // Point32 map_center( 224672.31, 5107378.91); // La Mola
  //Point32 map_center(235664.91, 5074788.07); // Tibidabo
  //Point32 map_center( 244808.69, 5070020.31); // bcn
  viewPort.setCenter(map_center);
  get_map_blocks(memBlocks, viewPort.bbox);
  draw(viewPort, memBlocks);

  // #ifdef DEFAULT_LAT
  //   load_main_screen();
  // #else
  //   lv_scr_load(searchSat);
  // #endif
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  while (gps->available() > 0)
  {
#ifdef OUTPUT_NMEA
    debug->write(gps->read());
#else
    GPS.encode(gps->read());
#endif
  }

#ifdef MAKERF_ESP32S3
  lv_tick_inc(5);
#endif
  //lv_timer_handler();
}
