/**
 * @file main.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ICENAV - ESP32 GPS Navigator main code
 * @version 0.3.0
 * @date 2026-06
 */

#include <Arduino.h>
#include <esp_log.h>

#include "hal.hpp"
#include "gps.hpp"
#include "storage.hpp"
#include "tft.hpp"
#include "connectivity.hpp"
#include "fileServer.hpp"
#include "battery.hpp"
#include "navContext.hpp"
#include "maps.hpp"
#include "diag.hpp"
#include "lv_subjects.hpp"

extern Storage storage;
extern Battery battery;
extern Maps mapView;

#include "timezone.c"
#include "settings.hpp"
#include "lvglSetup.hpp"
#include "tasks.hpp"

/**
 * @brief Initialize the ESP32 GPS Navigator system
 */
void setup()
{
    gpsMutex          = xSemaphoreCreateMutex();
    navCtx.routeMutex = xSemaphoreCreateMutex();
    sensorMutex       = xSemaphoreCreateMutex();
    lutInit = initTrigLUT();
    initHAL();
    bool sdOk     = (storage.initSD()     == ESP_OK);
    bool spiffsOk = (storage.initSPIFFS() == ESP_OK);
    if (!sdOk)
        ESP_LOGE("main", "SD card init failed — map data unavailable");
    if (!spiffsOk)
        ESP_LOGE("main", "SPIFFS init failed — assets unavailable");
    diagBootReport();
    battery.initADC();
    initTFT();
    createGpxFolders();
    mapView.initMap(tft.height() - 27, tft.width());
    loadPreferences();
    gps.init();
    initLVGL();
    if (!sdOk)
    {
        showMsg(LV_SYMBOL_WARNING, "SD card not found\nMap data unavailable");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    gps.gpsData.latitude = gps.getLat();
    gps.gpsData.longitude = gps.getLon();
    gps.publishSnapshot();
    initGpsTask();
    initSensorTask();
    initGuiTask();
    initNavTask();
    #ifndef DISABLE_CLI
        initCLI();
        initCLITask();
    #endif
    connectivity().begin();
    if (connectivity().isConnected() && enableWeb)
        fileServer().start();
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
        fileServer().process();

    vTaskDelay(pdMS_TO_TICKS(10));
}
