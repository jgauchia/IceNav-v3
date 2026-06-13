/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ICENAV - ESP32 GPS Navigator main code
 * @version 0.2.9
 * @date 2026-06
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <atomic>
#include <ESPmDNS.h>

#include "hal.hpp"
#include "gps.hpp"
#include "storage.hpp"
#include "tft.hpp"

extern xSemaphoreHandle gpsMutex;
#include "webpage.h"
#include "webserver.h"
#include "battery.hpp"
#include "power.hpp"
#include "gpxParser.hpp"
#include "climbAnalyzer.hpp"
#include "maps.hpp"
#include "lv_subjects.hpp"
#include "router.hpp"

extern Storage storage;
extern Battery battery;
extern Power power;
extern Maps mapView;

TrackVector trackData;
std::vector<TrackSegment> trackIndex;
std::vector<TurnPoint> turnPoints;
ClimbAnalyzer climbAnalyzer;

float                  routeDstLat       = 0.0f;
float                  routeDstLon       = 0.0f;
std::atomic<bool>      rerouteRequested  {false};
SemaphoreHandle_t      routeMutex        = nullptr;

#include "navigation.hpp"
NavState navState;
#include "timezone.c"
#include "settings.hpp"
#include "lvglSetup.hpp"
#include "tasks.hpp"

/**
 * @brief Initialize the ESP32 GPS Navigator system
 */
void setup()
{
    gpsMutex     = xSemaphoreCreateMutex();
    routeMutex   = xSemaphoreCreateMutex();
    sensorMutex  = xSemaphoreCreateMutex();
    lutInit = initTrigLUT();
    initHAL();
    storage.initSD();
    storage.initSPIFFS();
    battery.initADC();
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
    initGuiTask();
    initNavTask();
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
        configureWebServer();
    vTaskSuspend(guiTaskHandle);
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
    vTaskResume(guiTaskHandle);
}

/**
 * @brief Main application loop
 */
void loop()
{
    if (enableWeb)
        processWebServerTasks();

    vTaskDelay(pdMS_TO_TICKS(10));
}
