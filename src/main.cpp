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
#ifdef WAVESHARE_P4_35
    #include "axp2101.hpp"
    extern Axp2101 axp2101;
#endif
#ifdef BME280
    #include "bme.hpp"
    extern BME280_Driver bme;
#endif
#if defined(COMPASS_AUTO) && defined(WAVESHARE_P4_35)
    #include "compass.hpp"
    extern Compass compass;
#endif
#if defined(MPU6050) && defined(WAVESHARE_P4_35)
    #include "imu.hpp"
    extern MPU6050_Driver mpu;
#endif

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
    #ifdef WAVESHARE_P4_35
        // AXP2101 shares the touch I2C bus, already brought up by initTFT()
        // (LovyanGFX owns the port; see i2c_espidf's P4 guard).
        axp2101.begin(TOUCH_I2C_PORT);
    #endif
    #if defined(BME280) && defined(WAVESHARE_P4_35)
        // BME280 shares the same LovyanGFX-owned I2C bus as the touch
        // controller and the AXP2101 PMIC.
        bme.beginShared(TOUCH_I2C_PORT);
    #endif
    #if defined(COMPASS_AUTO) && defined(WAVESHARE_P4_35)
        // Compass shares the same LovyanGFX-owned I2C bus; chip (QMC5883L
        // or HMC5883L) is auto-detected at runtime.
        compass.initShared(TOUCH_I2C_PORT);
    #endif
    #if defined(MPU6050) && defined(WAVESHARE_P4_35)
        // IMU shares the same LovyanGFX-owned I2C bus.
        mpu.beginShared(TOUCH_I2C_PORT);
    #endif
    createGpxFolders();
    mapView.initMap(tft.height() - 27, tft.width());
    loadPreferences();
    gps.init();
    initLVGL();
    #if CONFIG_IDF_TARGET_ESP32P4
        // Warm up the LVGL flush/DMA path with a throwaway refresh before the
        // splash screen. Without this, the first real lv_refr_now (inside
        // lv_screen_load_anim) hangs forever in Bus_SPI::wait() — the SPI_USR
        // busy-wait never clears because the panel's DMA/flush state was never
        // armed by a prior transfer. Only reproduces with the SD card mounted.
        lv_obj_invalidate(lv_scr_act());
        lv_refr_now(display_drv);
    #endif
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
    connectivity().setPins();
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
