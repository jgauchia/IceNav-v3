/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ICENAV - ESP32 GPS Navigator main code
 * @version 0.2.0
 * @date 2025-04
 */

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_log.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SolarCalculator.h>

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

#ifdef MPU6050
#include "imu.hpp"
#endif

extern xSemaphoreHandle gpsMutex;

#include "webpage.h"
#include "webserver.h"
#include "battery.hpp"
#include "power.hpp"
#include "gpxParser.hpp"

#include "maps.hpp"

extern Storage storage;
extern Battery battery;
extern Power power;
extern Maps mapView;
extern Gps gps;
#ifdef ENABLE_COMPASS
Compass compass;
#endif

/**
 * @brief Sunrise and Sunset
 *
 */
static double transit, sunrise, sunset;

#include "timezone.c"
#include "settings.hpp" 
#include "lvglSetup.hpp"
#include "tasks.hpp"

/**
 * @brief Calculate Sunrise and Sunset
 *        Must be a global function
 *
 */
void calculateSun()
{
  calcSunriseSunset(2000 + fix.dateTime.year, 
                    fix.dateTime.month, 
                    fix.dateTime.date,
                    gps.gpsData.latitude, 
                    gps.gpsData.longitude,
                    transit, 
                    sunrise, 
                    sunset);
  int hours = (int)sunrise + gps.gpsData.UTC;
  int minutes = (int)round(((sunrise + gps.gpsData.UTC) - hours) * 60);
  snprintf(gps.gpsData.sunriseHour, 6, "%02d:%02d", hours, minutes);         
  hours = (int)sunset +  gps.gpsData.UTC;
  minutes = (int)round(((sunset +  gps.gpsData.UTC) - hours) * 60);
  snprintf(gps.gpsData.sunsetHour, 6, "%02d:%02d", hours, minutes); 
  log_i("Sunrise: %s",gps.gpsData.sunriseHour);
  log_i("Sunset: %s",gps.gpsData.sunsetHour);               
}

/**
 * @brief Setup
 *
 */
void setup()
{
  gpsMutex = xSemaphoreCreateMutex();
  esp_log_level_set("*", ESP_LOG_DEBUG);
  esp_log_level_set("storage", ESP_LOG_DEBUG);

// Force GPIO0 to internal PullUP  during boot (avoid LVGL key read)
#ifdef POWER_SAVE
  pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
#ifdef ICENAV_BOARD
  gpio_hold_dis((gpio_num_t)TFT_BL);
  gpio_hold_dis((gpio_num_t)BOARD_BOOT_PIN);
  gpio_deep_sleep_hold_dis();
#endif
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
  compass.init();
#endif

#ifdef ENABLE_IMU
  initIMU();
#endif
 
  storage.initSD();
  storage.initSPIFFS();
  battery.initADC();

  initTFT();

  mapView.initMap(TFT_HEIGHT - 100, TFT_WIDTH, TFT_HEIGHT);

  loadPreferences();
  gps.init();
  initLVGL();

  // Get init Latitude and Longitude
  gps.gpsData.latitude = gps.getLat();
  gps.gpsData.longitude = gps.getLon();

  initGpsTask();

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

  if (WiFi.getMode() == WIFI_OFF)
    ESP_ERROR_CHECK(esp_event_loop_create_default());

  splashScreen();
  lv_screen_load(searchSatScreen);
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

  // Deleting recursive directories in webfile server
  if (enableWeb && deleteDir)
  {
    deleteDir = false;
    if (deleteDirRecursive(deletePath.c_str()))
    {
      updateList = true;
      eventRefresh.send("refresh", nullptr, millis());
      eventRefresh.send("Folder deleted", "updateStatus", millis());
    }
  }
}
