/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1.8
 * @date 2024-04
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

unsigned long millisActual = 0;
static ulong lvglTickMillis = millis();

#include "hardware/hal.h"
#include "hardware/gps.h"
#include "utils/preferences.h"
#include "hardware/sdcard.h"
#include "hardware/tft.h"

#ifdef ENABLE_COMPASS
#include "hardware/compass.h"
#endif
#ifdef ENABLE_BME
#include "hardware/bme.h"
#endif
#include "hardware/battery.h"
#include "hardware/power.h"
#include "utils/gps_math.h"
#include "utils/sat_info.h"
#include "utils/lv_spiffs_fs.h"
#include "utils/lv_sd_fs.h"
#include "utils/time_zone.h"
#include "utils/render_maps.h"
#include "utils/vector_maps.h"
#include "gui/lvgl.h"

#include "tasks.h"

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
  initCompass();
#endif

  powerOn();
  loadPreferences();
  initSD();
  initSPIFFS();
  initTFT();
  initLVGL();
  initGPS();
  initADC();

  if (!isVectorMap)
  {
    mapSprite.deleteSprite();
    mapSprite.createSprite(768, 768);
  }

  splashScreen();
  initTasks();

//#ifdef DEFAULT_LAT
  loadMainScreen();
//#else
//  lv_screen_load(searchSat);
//#endif
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
    log_v("%s",gps->read());
#else
    GPS.encode(gps->read());
#endif
  }

  lv_timer_handler();
  unsigned long tick_millis = millis() - lvglTickMillis;
  lvglTickMillis = millis();
  lv_tick_inc(tick_millis);
  yield();
  delay(5);
}
