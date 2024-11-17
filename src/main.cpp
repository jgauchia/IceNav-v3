/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 GPS Navigation main code
 * @version 0.1.9_alpha
 * @date 2024-11
 */

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

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

#include "webpage.h"
#include "webserver.h"
#include "battery.hpp"

extern Storage storage;
extern Battery battery;

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
  
  // Force GPIO0 to internal PullUP  during boot (avoid LVGL key read)
  #ifdef POWER_SAVE
    pinMode(BOARD_BOOT_PIN,INPUT_PULLUP);
    #ifdef ICENAV_BOARD
      gpio_hold_dis((gpio_num_t)TFT_BL);
      gpio_hold_dis((gpio_num_t)BOARD_BOOT_PIN);
      gpio_deep_sleep_hold_dis();
    #endif
  #endif

  #ifdef ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);  
  #endif
  
  #ifdef TDECK_ESP32S3
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    pinMode(TCH_I2C_INT, INPUT);
    pinMode(SD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(TFT_SPI_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(TFT_SPI_CS, HIGH);
    pinMode(TFT_SPI_MISO, INPUT_PULLUP);
    pinMode(SD_MISO, INPUT_PULLUP);
  #endif

  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin();

  #ifdef BME280
   initBME();
  #endif

  #ifdef ENABLE_COMPASS
   initCompass();
  #endif

  powerOn();
  storage.initSD();
  storage.initSPIFFS();
  battery.initADC();
  initTFT();
  loadPreferences();
  initGPS();
  initLVGL();

  
  // Reserve PSRAM for buffer map
  mapTempSprite.deleteSprite();
  mapTempSprite.createSprite(TILE_WIDTH, TILE_HEIGHT);

  gpsData.latitude = getLat();
  gpsData.longitude = getLon();

  // Preload Map
  if (isVectorMap)
  {
  }
  else
  {
    // Get init Latitude and Longitude
    tileSize = RENDER_TILE_SIZE;
    generateRenderMap();
  }
  
  splashScreen();
  initGpsTask();

  lv_screen_load(searchSatScreen);

  #ifndef DISABLE_CLI
    initCLI();
    initCLITask();
  #endif

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!MDNS.begin(hostname))       
      log_e("nDNS init error");

    log_i("mDNS initialized");
  }

  if (WiFi.status() == WL_CONNECTED && enableWeb)
  {
    configureWebServer();
    server.begin();
  }

  if(WiFi.getMode() == WIFI_OFF)
    ESP_ERROR_CHECK(esp_event_loop_create_default());
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
