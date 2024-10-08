/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 GPS Navigation main code
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <SD.h>
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

#ifdef HMC5883L
#include "compass.hpp"
#endif

#ifdef QMC5883
#include "compass.hpp"
#endif

#ifdef IMU_MPU9250
#include "compass.hpp"
#endif

#ifdef BME280
#include "bme.hpp"
#endif

extern xSemaphoreHandle gpsMutex;

#include "battery.hpp"
#include "power.hpp"
#include "settings.hpp"
#include "lvglSetup.hpp"
#include "tasks.hpp"

/**
 * @brief Setup
 *
 */
void setup()
{
  gpsMutex = xSemaphoreCreateMutex();

  #ifdef ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);  
  #endif

  #ifdef ICENAV_BOARD
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
  #endif
  #ifdef MAKERF_ESP32S3
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
  #endif
  #ifdef ESP32S3_N16R8
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
  #endif
  #ifdef ELECROW_ESP32
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
  #endif

  #ifdef BME280
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
  
  // Reserve PSRAM for buffer map
  mapTempSprite.deleteSprite();
  mapTempSprite.createSprite(TILE_WIDTH, TILE_HEIGHT);

  splashScreen();
  initGpsTask();

  #ifdef DEFAULT_LAT
    loadMainScreen();
  #else
    lv_screen_load(searchSatScreen);
  #endif

  #ifndef DISABLE_CLI
    initCLI();
    initCLITask();
  #endif

  // Preload Map
  if (isVectorMap)
  {
  }
  else
  {
    tileSize = RENDER_TILE_SIZE;
    generateRenderMap();
  }
}

/**
 * @brief Main Loop
 *
 */
void loop()
{
  if (!waitScreenRefresh)
  {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(TASK_SLEEP_PERIOD_MS));
  }
}
