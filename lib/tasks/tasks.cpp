/**
 * @file tasks.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks implementation for GPS and CLI management
 * @version 0.2.5
 * @date 2026-04
 * @details This file contains the implementation of FreeRTOS tasks for GPS data processing
 *          and CLI interface management. It handles thread-safe GPS data reading and
 *          command-line interface operations with proper mutex protection.
 */

#include "tasks.hpp"
#include "mainScr.hpp"
#include "lv_subjects.hpp"
#include <WiFi.h>

xSemaphoreHandle gpsMutex;
extern Gps gps;
SensorData globalSensorData;

static const char* TAG = "Task";

/**
 * @brief GPS data processing task
 *
 * @details Continuously reads GPS data from the serial port, processes NMEA sentences,
 *          and updates the global GPS fix structure. Handles optional NMEA output to
 *          serial console and ensures thread-safe access using gpsMutex. The task runs
 *          on core 0 with high priority to ensure real-time GPS data processing.
 *
 * @param pvParameters Task parameters (unused in current implementation)
 */
void gpsTask(void *pvParameters)
{
    ESP_LOGV(TAG, "GPS Task - running on core %d", xPortGetCoreID());
    ESP_LOGV(TAG, "Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
    while (1)
    {
        if ( xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(100)) == pdTRUE )
        {
            if (nmea_output_enable)
            {
                while (gpsPort.available())
                {
                    char c = gpsPort.read();
                    Serial.print(c);
                }
            } 

            while (GPS.available( gpsPort )) 
            {
                fix = GPS.read();
                gps.getGPSData();
                
                if (isMainScreen && !canMoveWidget && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
                {
                    lv_subject_set_int(&subject_speed, (int32_t)gps.gpsData.speed);
                    lv_subject_set_int(&subject_altitude, (int32_t)gps.gpsData.altitude);
                    lv_subject_set_int(&subject_lat, (int32_t)(gps.gpsData.latitude * 1000000.0f));
                    lv_subject_set_int(&subject_lon, (int32_t)(gps.gpsData.longitude * 1000000.0f));
                    lv_subject_set_int(&subject_sats, (int32_t)gps.gpsData.satellites);
                    lv_subject_set_int(&subject_pdop, (int32_t)(gps.gpsData.pdop * 10.0f));
                    lv_subject_set_int(&subject_hdop, (int32_t)(gps.gpsData.hdop * 10.0f));
                    lv_subject_set_int(&subject_vdop, (int32_t)(gps.gpsData.vdop * 10.0f));
                    lv_subject_set_int(&subject_sats_data_trigger, lv_subject_get_int(&subject_sats_data_trigger) + 1);
                    lv_subject_set_int(&subject_fix_mode, (int32_t)gps.gpsData.fixMode);
                    lv_subject_set_int(&subject_is_fixed, isGpsFixed ? 1 : 0);
                    if (!mapSet.mapRotationComp)
                        lv_subject_set_int(&subject_heading, (int32_t)gps.gpsData.heading);
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            xSemaphoreGive(gpsMutex);
        }
        vTaskDelay(1);
    }
}

/**
 * @brief Initialize GPS processing task
 *
 * @details Creates and starts the GPS task on core 0 with 3KB stack size and priority 2.
 *          Includes a 500ms delay after task creation to ensure proper initialization
 *          before other system components attempt to access GPS data.
 */
void initGpsTask()
{
    xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 3072, NULL, 2, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

/**
 * @brief Command-line interface processing task
 *
 * @details Handles CLI operations including command parsing, execution, and response
 *          generation. Runs on core 1 with 3KB stack size to handle complex CLI
 *          operations and network communications. The task processes commands at
 *          60ms intervals to maintain responsive user interaction.
 *
 * @param param Task parameters (unused in current implementation)
 */
#ifndef DISABLE_CLI
void cliTask(void *param) 
{
    ESP_LOGV(TAG, "CLI Task - running on core %d", xPortGetCoreID());
    ESP_LOGV(TAG, "Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
    while(1) 
    {
        wcli.loop();
        vTaskDelay(60 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @brief Initialize CLI processing task
 *
 * @details Creates and starts the CLI task on core 1 with 3KB stack size and priority 1.
 *          Only compiled when CLI functionality is enabled (not DISABLE_CLI).
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 3072, NULL, 1, NULL, 1); }

#endif

#ifdef BME280
    extern BME280_Driver bme;
#endif
    extern Battery battery;
#ifdef ENABLE_COMPASS
    extern Compass compass;
#endif

/**
 * @brief Sensor data processing task
 *
 * @details Periodically reads data from non-GPS sensors (BME280, Compass, Battery)
 *          and updates the global globalSensorData structure.
 *          The compass is sampled at a high rate (50Hz) for UI fluidity, while
 *          the battery and BME are sampled slowly (~1.5s).
 *          Sensor readings are suspended during map scrolling to free up the I2C bus.
 *
 * @param pvParameters Task parameters (unused)
 */
void sensorTask(void *pvParameters)
{
    uint16_t slowCounter = 0;
    static time_t lastTimeSent = 0;

    while (1)
    {
        if (isScrollingMap || canMoveWidget)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; 
        }

        #ifdef ENABLE_COMPASS
            globalSensorData.heading = compass.getHeading();
            if (isMainScreen && !canMoveWidget && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
            {
                if (mapSet.mapRotationComp)
                    lv_subject_set_int(&subject_heading, globalSensorData.heading);
                xSemaphoreGive(lvgl_mutex);
            }
        #endif

        // Update time subject once per second (from reliable sensorTask loop)
        time_t now = time(NULL);
        if (now != lastTimeSent)
        {
            if (isMainScreen && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                lv_subject_set_int(&subject_time, (int32_t)now);
                lv_subject_notify(&subject_time);
                lastTimeSent = now;
                xSemaphoreGive(lvgl_mutex);
            }
        }

        if (slowCounter++ >= 75) 
        {
            #ifdef BME280
                bme.readAll(globalSensorData.temperature, globalSensorData.pressure, globalSensorData.humidity);
                globalSensorData.altitude = (int16_t)bme.readAltitude(globalSensorData.pressure);
            #endif
            
            #ifdef ENABLE_TEMP
            static uint8_t lastTempSent = 255;
            uint8_t currentTemp = (uint8_t)(globalSensorData.temperature + tempOffset);
            if (isMainScreen && currentTemp != lastTempSent)
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
                {
                    lv_subject_set_int(&subject_temp, (int32_t)currentTemp);
                    lastTempSent = currentTemp;
                    xSemaphoreGive(lvgl_mutex);
                }
            }
            #endif

            static bool lastWifiState = false;
            bool currentWifiState = (WiFi.status() == WL_CONNECTED);
            if (currentWifiState != lastWifiState)
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
                {
                    lv_subject_set_int(&subject_wifi, currentWifiState ? 1 : 0);
                    lastWifiState = currentWifiState;
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            globalSensorData.batteryPercent = battery.readBattery();
            static int lastSentValue = -1;
            int current = (int)globalSensorData.batteryPercent;
            
            auto getLevel = [](int v) 
            {
                if (v > 110) 
                    return 5;
                if (v > 80)  
                    return 4;
                if (v > 60)  
                    return 3;
                if (v > 40)  
                    return 2;
                if (v > 20)  
                    return 1;
                return 0;
            };

            bool thresholdCrossed = getLevel(current) != getLevel(lastSentValue);
            bool significantChange = abs(current - lastSentValue) >= 3;

            if (isMainScreen && !canMoveWidget && (thresholdCrossed || significantChange))
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
                {
                    lv_subject_set_int(&subject_battery, (int32_t)current);
                    lastSentValue = current;
                    xSemaphoreGive(lvgl_mutex);
                }
            }
            slowCounter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * @brief Initialize sensor processing task
 *
 * @details Creates and starts the sensor task on core 0 with 3KB stack and priority 1.
 */
void initSensorTask()
{
    xTaskCreatePinnedToCore(sensorTask, "Sensor Task", 3072, NULL, 1, NULL, 0);
}

/**
 * @brief GUI management task
 *
 * @details Handles the LVGL timer handler and UI refresh logic. It runs on core 1
 *          with high priority to ensure smooth user interaction. Includes adaptive
 *          sleep logic to save power when the UI is inactive or the device is stationary.
 *
 * @param pvParameters Task parameters (unused)
 */
void guiTask(void *pvParameters)
{
    while (1)
    {
        uint32_t wait_ms = 10;
        if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE)
        {
            wait_ms = lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        }
        
        if (wait_ms > 100) 
            wait_ms = 100;
        if (wait_ms < 5) 
            wait_ms = 5;
        
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

/**
 * @brief Initialize GUI management task
 *
 * @details Creates and starts the GUI task on core 1 with 8KB stack and priority 3.
 *          This ensures that UI updates and touch events are processed with the
 *          highest application priority.
 */
void initGuiTask()
{
    xTaskCreatePinnedToCore(guiTask, "GUI Task", 8192, NULL, 3, NULL, 1);
}
