/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ICENAV - ESP32 GPS Navigator main code
 * @version 0.2.4
 * @date 2025-12
 */

#include <Arduino.h>
#include "i2c_espidf.hpp"
#include <WiFi.h>
#include <esp_log.h>
#include <ESPmDNS.h>
#include <SolarCalculator.h>

int taskSleepPeriod = 10;

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
#ifdef ENABLE_COMPASS
    Compass compass;
#endif

TrackVector trackData;
std::vector<TrackSegment> trackIndex;
std::vector<TurnPoint> turnPoints;

#include "navigation.hpp"
NavState navState;
static double transit, sunrise, sunset;

#include "timezone.c"
#include "settings.hpp" 
#include "lvglSetup.hpp"
#include "tasks.hpp"

/**
 * @brief Calculate Sunrise and Sunset based on current GPS position and date.
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
 * @brief Initialize the ESP32 GPS Navigator system
 */
void setup()
{
    gpsMutex = xSemaphoreCreateMutex();
    lutInit = initTrigLUT();
    #ifdef POWER_SAVE
        pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
        #ifdef ICENAV_BOARD
            gpio_hold_dis(GPIO_NUM_46);
            gpio_hold_dis((gpio_num_t)BOARD_BOOT_PIN);
            gpio_deep_sleep_hold_dis();
        #endif
    #endif
    #ifdef TDECK_ESP32S3
        pinMode(BOARD_POWERON, OUTPUT);
        digitalWrite(BOARD_POWERON, HIGH);
        pinMode(GPIO_NUM_16, INPUT);
        pinMode(SD_CS, OUTPUT);
        pinMode(RADIO_CS_PIN, OUTPUT);
        pinMode(TFT_SPI_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);
        digitalWrite(RADIO_CS_PIN, HIGH);
        digitalWrite(TFT_SPI_CS, HIGH);
        pinMode(SPI_MISO, INPUT_PULLUP);
    #endif
    i2c.begin(I2C_SDA_PIN, I2C_SCL_PIN);
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
    #ifdef ENABLE_COMPASS
        vTaskDelay(pdMS_TO_TICKS(50));
    #endif
    initTFT();
    createGpxFolders();
    mapView.initMap(tft.height() - 27, tft.width());
    loadPreferences();
    gps.init();
    initLVGL();
    gps.gpsData.latitude = gps.getLat();
    gps.gpsData.longitude = gps.getLon();
    initGpsTask();
    initSensorTask();
    #ifndef DISABLE_CLI
        initCLI();
        initCLITask();
    #endif
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!MDNS.begin(hostname)) log_e("nDNS init error");
        log_i("mDNS initialized");
    }
    if (WiFi.status() == WL_CONNECTED && enableWeb) configureWebServer();
    splashScreen();
    if (isGpsFixed)
    {
        isSearchingSat = false;
        loadMainScreen();
    }
    else
    {
        lv_timer_resume(searchTimer);
        lv_screen_load(searchSatScreen);
    }
}

/**
 * @brief Main application loop
 */
void loop()
{
    lv_timer_handler();
    if (gps.gpsData.speed > 0) taskSleepPeriod = 20;
    else taskSleepPeriod = 50;
    if (lv_disp_get_inactive_time(NULL) < 1000) taskSleepPeriod = 10;
    vTaskDelay(pdMS_TO_TICKS(taskSleepPeriod));

    if (enableWeb) processWebServerTasks();
    if (isTrackLoaded)
    {
        if (navSet.simNavigation) gps.simFakeGPS(trackData,120,1000);
        if (gps.gpsData.speed !=0)
        {
            NavConfig simConfig;
            simConfig.searchWindow = 150;
            simConfig.offTrackThreshold = 75.0f;
            simConfig.maxBackwardJump = 10;
            static unsigned long lastNavUpdate = 0;
            if (millis() - lastNavUpdate > 100)
            {
                lastNavUpdate = millis();
                updateNavigation(gps.gpsData.latitude, gps.gpsData.longitude, gps.gpsData.heading, gps.gpsData.speed,
                                trackData, turnPoints, navState, 20, 200, simConfig);
            }
        }
    }
}
