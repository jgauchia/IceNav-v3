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
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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

#include "webpage.h"
#include "webserver.h"
#include "battery.hpp"
#include "power.hpp"
#include "settings.hpp"
#include "lvglSetup.hpp"
#include "tasks.hpp"

extern uint32_t deviceSuspendCount;

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
  #endif

  #ifdef ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);  
  #endif
  
  #ifdef TDECK_ESP32S3
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    pinMode(TCH_I2C_INT, INPUT);
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    pinMode(SD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(TFT_SPI_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(TFT_SPI_CS, HIGH);
    pinMode(TFT_SPI_MISO, INPUT_PULLUP);
    pinMode(SD_MISO, INPUT_PULLUP);
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI);
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
  initSD();
  initSPIFFS();
  initTFT();
  loadPreferences();
  initGPS();
  initLVGL();
  initADC();
  
  // Reserve PSRAM for buffer map
  mapTempSprite.deleteSprite();
  mapTempSprite.createSprite(TILE_WIDTH, TILE_HEIGHT);

  // Preload Map
  if (isVectorMap)
  {
  }
  else
  {
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
  if (deviceSuspendCount == 500) deviceShutdown();
  if (deviceSuspendCount == 1) deviceSuspend();
  if (deviceSuspendCount > 0 ) deviceSuspendCount--;
}
