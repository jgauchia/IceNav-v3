/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 GPS Naviation main code
 * @version 0.1.8
 * @date 2024-05
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
  initBME();
#endif

#ifdef ENABLE_COMPASS
  initCompass();
#endif

  powerOn();
  loadPreferences();
  initSD();
  initSPIFFS();
  initTFT();
  initGPS();
  initLVGL();
  initADC();
  initTasks();

  // Reserve PSRAM if rendered map is selected
  // Create a Sprite por temporary 9x9 tile map
  if (!isVectorMap)
  {
    mapTempSprite.deleteSprite();
    mapTempSprite.createSprite(768, 768);
  }

  splashScreen();

#ifdef DEFAULT_LAT
  loadMainScreen();
#else
  lv_screen_load(searchSatScreen);
#endif
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  // lv_timer_handler();
  // lv_tick_inc(5);
  // lv_timer_handler(); /* let the GUI do its work */
  // vTaskDelay(pdMS_TO_TICKS(TASK_SLEEP_PERIOD_MS));
}
