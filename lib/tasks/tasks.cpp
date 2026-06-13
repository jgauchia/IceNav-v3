/**
 * @file tasks.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks implementation for GPS and CLI management
 * @version 0.2.9
 * @date 2026-06
 * @details This file contains the implementation of FreeRTOS tasks for GPS data processing
 *          and CLI interface management. It handles thread-safe GPS data reading and
 *          command-line interface operations with proper mutex protection.
 */

#include "tasks.hpp"
#include "mainScr.hpp"
#include "lv_subjects.hpp"
#include "router.hpp"
#include "gpxParser.hpp"
#include <WiFi.h>
#ifdef ENABLE_IMU
    #include "imu.hpp"
#endif

xSemaphoreHandle gpsMutex;
SemaphoreHandle_t sensorMutex = NULL;
TaskHandle_t gpsTaskHandle = NULL;
extern Gps gps;
SensorData globalSensorData = {};

// Debug NMEA stats — written by gpsTask, read by GUI
uint32_t nmeaDebugOk     = 0;
uint32_t nmeaDebugErr    = 0;
uint32_t nmeaDebugCycles = 0;
uint8_t  nmeaLastMsg     = 0;   // last GPS.nmeaMessage seen per cycle
portMUX_TYPE nmeaDebugMux = portMUX_INITIALIZER_UNLOCKED;

// Raw NMEA sentence ring buffer — written by gpsTask, read by debug tile
char    nmeaRawBuf[NMEA_RAW_LINES][NMEA_RAW_LEN] = {};
uint8_t nmeaRawHead = 0;

static constexpr TickType_t MUTEX_TIMEOUT_GPS  = pdMS_TO_TICKS(15);
static constexpr TickType_t MUTEX_TIMEOUT_SLOW = pdMS_TO_TICKS(10);

static const char* TAG = "Task";

static struct
{
    time_t  lastTimeSent  = 0;
    uint8_t lastTempSent  = 255;
    bool    lastWifiState = false;
    int     lastSentValue = -1;
} sensorState;

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
    char    lineBuf[NMEA_RAW_LEN] = {};
    uint8_t lineLen = 0;
    while (1)
    {
        if ( gpsMutex != NULL && xSemaphoreTake(gpsMutex, MUTEX_TIMEOUT_GPS) == pdTRUE )
        {
            bool satDataUpdated = false;

            if (nmea_output_enable)
            {
                while (gpsPort.available())
                    Serial.write(gpsPort.read());
            }
            else
            {
                // Feed the parser one character at a time (same bytes, same order
                // as GPS.available() would consume) while mirroring each complete
                // sentence into the raw ring buffer for the debug tile. Parsing is
                // unaffected; capture is purely a side effect.

                while (gpsPort.available())
                {
                    char c = (char)gpsPort.read();

                    if (c == '\n' || c == '\r')
                    {
                        if (lineLen > 0)
                        {
                            lineBuf[lineLen] = '\0';
                            portENTER_CRITICAL(&nmeaDebugMux);
                            strncpy(nmeaRawBuf[nmeaRawHead], lineBuf, NMEA_RAW_LEN - 1);
                            nmeaRawBuf[nmeaRawHead][NMEA_RAW_LEN - 1] = '\0';
                            nmeaRawHead = (nmeaRawHead + 1) % NMEA_RAW_LINES;
                            portEXIT_CRITICAL(&nmeaDebugMux);
                            lineLen = 0;
                        }
                    }
                    else if (lineLen < NMEA_RAW_LEN - 1)
                    {
                        lineBuf[lineLen++] = c;
                    }

                    // handle() returns DECODE_COMPLETED at the end of every
                    // sentence, but a fix is only buffered once the interval
                    // closes (LAST_SENTENCE_IN_INTERVAL). Read only when one is
                    // actually available, exactly like GPS.available() does.
                    if (GPS.handle((uint8_t)c) == NMEAGPS::DECODE_COMPLETED && GPS.available())
                    {
                        fix = GPS.read();
                        gps.getGPSData();

                        portENTER_CRITICAL(&nmeaDebugMux);
                        nmeaDebugOk    = GPS.statistics.ok;
                        nmeaDebugErr   = GPS.statistics.errors;
                        nmeaLastMsg    = (uint8_t)GPS.nmeaMessage;
                        nmeaDebugCycles++;
                        portEXIT_CRITICAL(&nmeaDebugMux);

                        // Only flag a satellite-data update when this cycle actually
                        // carried GSV (sat_count > 0). With GSV decimated to ~1Hz the
                        // intermediate cycles report 0, so this avoids redrawing the
                        // constellation at the full rate with unchanged data.
                        if (GPS.sat_count > 0)
                            satDataUpdated = true;
                    }
                }
            }

            /* Non-blocking: gpsTask (core 0) never waits on the GUI task (core 1).
               If lvgl_mutex is taken, subject updates are skipped for this cycle.
               Intentional — the GPS task must not stall waiting for LVGL. */
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
                lv_subject_set_int(&subject_fix_mode, (int32_t)gps.gpsData.fixMode);
                lv_subject_set_int(&subject_is_fixed, isGpsFixed ? 1 : 0);
                if (!mapSet.mapRotationComp)
                    lv_subject_set_int(&subject_heading, (int32_t)gps.gpsData.heading);
                xSemaphoreGive(lvgl_mutex);
            }

            if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
            {
                if (satDataUpdated)
                    lv_subject_set_int(&subject_sats_data_trigger, lv_subject_get_int(&subject_sats_data_trigger) + 1);
                lv_subject_set_int(&subject_nmea_debug_trigger, lv_subject_get_int(&subject_nmea_debug_trigger) + 1);
                xSemaphoreGive(lvgl_mutex);
            }

            xSemaphoreGive(gpsMutex);
        }
        vTaskDelay(1);
    }
}

/**
 * @brief Initialize GPS processing task
 *
 * @details Creates and starts the GPS task on core 0 with 4KB stack size and priority 2.
 *          Includes a 500ms delay after task creation to ensure proper initialization
 *          before other system components attempt to access GPS data.
 */
void initGpsTask()
{
    xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 4096, NULL, 2, &gpsTaskHandle, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

/**
 * @brief Command-line interface processing task
 *
 * @details Handles CLI operations including command parsing, execution, and response
 *          generation. Runs on core 1 with 16KB stack size to handle complex CLI
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
    while (1)
    {
        wcli.loop();
        vTaskDelay(60 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @brief Initialize CLI processing task
 *
 * @details Creates and starts the CLI task on core 1 with 16KB stack size and priority 1.
 *          Only compiled when CLI functionality is enabled (not DISABLE_CLI).
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 16384, NULL, 1, NULL, 1); }

#endif

#ifdef BME280
    extern BME280_Driver bme;
#endif
    extern Battery battery;
#ifdef ENABLE_COMPASS
    extern Compass compass;
#endif

static int getBatteryLevel(int v)
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
}

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

    while (1)
    {
        if (isScrollingMap || canMoveWidget)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        #ifdef ENABLE_COMPASS
        {
            int newHeading = compass.getHeading();
            if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
            {
                globalSensorData.heading = newHeading;
                xSemaphoreGive(sensorMutex);
            }
            /* Non-blocking: same rationale as gpsTask — sensor readings are
               best-effort; if LVGL holds the mutex this cycle is discarded. */
            if (isMainScreen && !canMoveWidget && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
            {
                lv_subject_set_int(&subject_compass_heading, newHeading);
                if (mapSet.mapRotationComp)
                    lv_subject_set_int(&subject_heading, newHeading);
                xSemaphoreGive(lvgl_mutex);
            }
        }
        #endif

        // Update time subject once per second (from reliable sensorTask loop)
        time_t now = time(NULL);
        if (now != sensorState.lastTimeSent)
        {
            if (isMainScreen && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
            {
                lv_subject_set_int(&subject_time, (int32_t)now);
                lv_subject_notify(&subject_time);
                sensorState.lastTimeSent = now;
                xSemaphoreGive(lvgl_mutex);
            }
        }

        if (slowCounter++ >= 25)
        {
            #ifdef BME280
            {
                float t = 0.0f;
                float p = 0.0f;
                float h = 0.0f;
                bme.readAll(t, p, h);
                int16_t alt = (int16_t)bme.readAltitude(p);
                if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
                {
                    globalSensorData.temperature = t;
                    globalSensorData.pressure    = p;
                    globalSensorData.humidity    = h;
                    globalSensorData.altitude    = alt;
                    xSemaphoreGive(sensorMutex);
                }
            }
            #endif

            #ifdef ENABLE_TEMP
            uint8_t currentTemp = (uint8_t)(globalSensorData.temperature + tempOffset);
            if (isMainScreen && currentTemp != sensorState.lastTempSent)
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, MUTEX_TIMEOUT_SLOW) == pdTRUE)
                {
                    lv_subject_set_int(&subject_temp, (int32_t)currentTemp);
                    sensorState.lastTempSent = currentTemp;
                    xSemaphoreGive(lvgl_mutex);
                }
            }
            #endif

            bool currentWifiState = (WiFi.status() == WL_CONNECTED);
            if (currentWifiState != sensorState.lastWifiState)
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, MUTEX_TIMEOUT_SLOW) == pdTRUE)
                {
                    lv_subject_set_int(&subject_wifi, currentWifiState ? 1 : 0);
                    sensorState.lastWifiState = currentWifiState;
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            float rawBattery = battery.readBattery();
            if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
            {
                globalSensorData.batteryPercent = rawBattery;
                globalSensorData.batteryVoltage = battery.lastVoltage();
                xSemaphoreGive(sensorMutex);
            }

            #ifdef ENABLE_IMU
            {
                float ax = 0.0f;
                float ay = 0.0f;
                float az = 0.0f;
                float gx = 0.0f;
                float gy = 0.0f;
                float gz = 0.0f;
                float imuTemp = 0.0f;
                mpu.readAll(ax, ay, az, gx, gy, gz, imuTemp);
                if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
                {
                    globalSensorData.accelX = ax;
                    globalSensorData.accelY = ay;
                    globalSensorData.accelZ = az;
                    globalSensorData.gyroX  = gx;
                    globalSensorData.gyroY  = gy;
                    globalSensorData.gyroZ  = gz;
                    xSemaphoreGive(sensorMutex);
                }
            }
            #endif

            int current = (int)rawBattery;

            bool thresholdCrossed = getBatteryLevel(current) != getBatteryLevel(sensorState.lastSentValue);
            bool significantChange = abs(current - sensorState.lastSentValue) >= 3;

            if (isMainScreen && !canMoveWidget && (thresholdCrossed || significantChange))
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, 0) == pdTRUE)
                {
                    lv_subject_set_int(&subject_battery, (int32_t)current);
                    sensorState.lastSentValue = current;
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
TaskHandle_t guiTaskHandle = NULL;

void initGuiTask()
{
    xTaskCreatePinnedToCore(guiTask, "GUI Task", 8192, NULL, 3, &guiTaskHandle, 1);
}

extern TrackVector            trackData;
extern std::vector<TurnPoint> turnPoints;
extern NavState               navState;
extern float                  routeDstLat;
extern float                  routeDstLon;
extern std::atomic<bool>      rerouteRequested;
extern SemaphoreHandle_t      routeMutex;
extern Maps                   mapView;

/**
 * @brief Navigation and routing task
 *
 * @details Handles route recalculation when rerouteRequested is set, then continuously
 *          runs turn-by-turn navigation updates at 10 Hz when a track is loaded and
 *          the vehicle is moving. Route calculation (A*) and navigation updates both
 *          run on core 1 alongside the GUI task; routeMutex protects shared track data.
 *
 * @param pvParameters Task parameters (unused)
 */
void navTask(void *pvParameters)
{
    ESP_LOGV(TAG, "Nav Task - running on core %d", xPortGetCoreID());

    static NavConfig navConfig;
    navConfig.searchWindow      = 150;
    navConfig.offTrackThreshold = 75.0f;
    navConfig.maxBackwardJump   = 10;

    static unsigned long lastNavUpdate = 0;

    while (1)
    {
        if (rerouteRequested.exchange(false))
        {
            if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE)
            {
                showMsg(LV_SYMBOL_REFRESH, " Calculating route...");
                xSemaphoreGive(lvgl_mutex);
            }

            TrackVector newRoute;
            RouterResult res = router.route(gps.gpsData.latitude, gps.gpsData.longitude,
                                            routeDstLat, routeDstLon, newRoute);

            if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                closeMsg();
                xSemaphoreGive(lvgl_mutex);
            }

            if (res == RouterResult::OK)
            {
                isTrackLoaded = false;
                trackData.clear();
                trackData.shrink_to_fit();

                if (xSemaphoreTake(routeMutex, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    trackData = std::move(newRoute);

                    if (!trackData.empty())
                    {
                        trackData[0].accumDist = 0.0f;
                        for (size_t i = 1; i < trackData.size(); ++i)
                        {
                            float d = calcDist(trackData[i-1].lat, trackData[i-1].lon,
                                               trackData[i].lat,   trackData[i].lon);
                            trackData[i].accumDist = trackData[i-1].accumDist + d;
                        }
                    }

                    GPXParser gpxTmp;
                    turnPoints    = gpxTmp.getTurnPointsSlidingWindow(10.0f, 5, 45.0f, 3, trackData);
                    navState      = NavState{};
                    gps.resetSimulation();
                    resetNavigationUI();
                    isTrackLoaded = !trackData.empty();
                    xSemaphoreGive(routeMutex);
                }

                mapView.updateMap();
                mapView.redrawTrack();
            }

            if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                lv_subject_set_int(&subject_rerouting, 0);
                lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
                lv_obj_clear_flag(turnByTurn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
                xSemaphoreGive(lvgl_mutex);
            }
        }

        if (isTrackLoaded)
        {
            if (navSet.simNavigation)
            {
                float oldLat = gps.gpsData.latitude;
                gps.simFakeGPS(trackData, 40, 500);
                if (gps.gpsData.latitude != oldLat && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    lv_subject_set_int(&subject_lat,     (int32_t)(gps.gpsData.latitude  * 1000000.0f));
                    lv_subject_set_int(&subject_lon,     (int32_t)(gps.gpsData.longitude * 1000000.0f));
                    lv_subject_set_int(&subject_heading, (int32_t)gps.gpsData.heading);
                    lv_subject_set_int(&subject_speed,   (int32_t)gps.gpsData.speed);
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            if (gps.gpsData.speed != 0 || navSet.simNavigation)
            {
                unsigned long now = (unsigned long)(esp_timer_get_time() / 1000ULL);
                if (now - lastNavUpdate > 100)
                {
                    lastNavUpdate = now;
                    if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
                    {
                        updateNavigation(gps.gpsData.latitude, gps.gpsData.longitude,
                                         gps.gpsData.heading,  gps.gpsData.speed,
                                         trackData, turnPoints, navState,
                                         20, 200, navConfig);
                        xSemaphoreGive(lvgl_mutex);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Initialize navigation task
 *
 * @details Creates and starts the nav task on core 1 with 6KB stack and priority 1.
 */
void initNavTask()
{
    xTaskCreatePinnedToCore(navTask, "Nav Task", 6144, NULL, 2, NULL, 1);
}
