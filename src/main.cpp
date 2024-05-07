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
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Timezone.h>

static ulong lvglTickMillis = millis();

// Hardware includes
#include "hal.hpp"
#include "gps.hpp"
#include "storage.hpp"
#include "tft.hpp"
#ifdef ENABLE_COMPASS
#include "compass.hpp"
#endif
#ifdef ENABLE_BME
#include "bme.hpp"
#endif
#include "battery.hpp"
#include "power.hpp"
#include "settings.hpp"

#include "tasks.hpp"
#include "satInfo.hpp"
#include "gpsMath.hpp"
#include "vectorMaps.hpp"
#include "renderMaps.hpp"

#include "lvglSetup.hpp"

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
  repeatCalib = true;
  powerOn();
  loadPreferences();
  initSD();
  initSPIFFS();
  initTFT();
  initGPS();
  initLVGL();
  initADC();

  if (!isVectorMap)
  {
    mapSprite.deleteSprite();
    mapSprite.createSprite(768, 768);
  }

  splashScreen();
  initTasks();

  // #ifdef DEFAULT_LAT
  loadMainScreen();
  // #else
  //   lv_screen_load(searchSatScreen);
  // #endif
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  lv_timer_handler();
  unsigned long tick_millis = millis() - lvglTickMillis;
  lvglTickMillis = millis();
  lv_tick_inc(tick_millis);
  yield();
  delay(5);
}
