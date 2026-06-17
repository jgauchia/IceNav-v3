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
#include "logger.hpp"
#include "router.hpp"
#include "gpxParser.hpp"
#include "connectivity.hpp"
#include "sensors.hpp"
#ifdef ENABLE_IMU
    #include "imu.hpp"
#endif

xSemaphoreHandle gpsMutex;
SemaphoreHandle_t sensorMutex = NULL;
TaskHandle_t gpsTaskHandle    = NULL;
TaskHandle_t guiTaskHandle    = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t navTaskHandle    = NULL;
#ifndef DISABLE_CLI
TaskHandle_t cliTaskHandle    = NULL;
#endif
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
    // Block-read scratch: sized to ~one NMEA sentence (NMEA_RAW_LEN = 88) with
    // margin. The drain loop below reads repeatedly, so this only bounds how many
    // bytes are handled per read() call, not how many can be processed per cycle;
    // any larger backlog (driver RX buffer is 2048 B) is consumed in later passes.
    uint8_t readBuf[128];
    while (1)
    {
        bool satDataUpdated = false;

        // Read the serial port and feed the parser WITHOUT holding gpsMutex.
        // Parsing and raw capture only touch the NMEA decoder and the debug ring
        // buffer; the published gps.gpsData snapshot is written under gpsMutex
        // only when a fix is actually decoded (below). This keeps the GUI from
        // ever waiting on the byte-level parsing loop.
        if (nmea_output_enable)
        {
            int n;
            while ((n = gpsPort.read(readBuf, sizeof(readBuf))) > 0)
                Serial.write(readBuf, n);
        }
        else
        {
            // Raw sentence capture for the debug tile is only needed while that
            // tile is on screen. When it is not, skip building lineBuf and its
            // per-sentence critical section entirely. Reset any partial line so
            // re-entering the tile mid-sentence cannot stitch a stale fragment
            // onto the next captured sentence.
            const bool captureRaw = (activeTile == DEBUG_NMEA);
            if (!captureRaw)
                lineLen = 0;

            // Read in blocks and feed the parser byte by byte from the local
            // buffer (same bytes, same order a per-byte read() would yield).
            int n;
            while ((n = gpsPort.read(readBuf, sizeof(readBuf))) > 0)
            {
                for (int i = 0; i < n; i++)
                {
                    char c = (char)readBuf[i];

                    if (captureRaw)
                    {
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
                    }

                    // handle() returns DECODE_COMPLETED at the end of every
                    // sentence, but a fix is only buffered once the interval
                    // closes (LAST_SENTENCE_IN_INTERVAL). Read only when one is
                    // actually available, exactly like GPS.available() does.
                    if (GPS.handle((uint8_t)c) == NMEAGPS::DECODE_COMPLETED && GPS.available())
                    {
                        fix = GPS.read();

                        if (gpsMutex != NULL && xSemaphoreTake(gpsMutex, MUTEX_TIMEOUT_GPS) == pdTRUE)
                        {
                            gps.getGPSData();
                            {
                                LoggerGpsFix lgf;
                                lgf.lat      = gps.gpsData.latitude;
                                lgf.lon      = gps.gpsData.longitude;
                                lgf.alt      = (int16_t)gps.gpsData.altitude;
                                lgf.speedKmh = (float)gps.gpsData.speed;
                                lgf.valid    = isGpsFixed && fix.valid.location;
                                lgf.hasTime  = fix.valid.time && fix.valid.date;
                                lgf.year     = (uint16_t)(2000 + fix.dateTime.year);
                                lgf.month    = fix.dateTime.month;
                                lgf.day      = fix.dateTime.date;
                                lgf.hour     = fix.dateTime.hours;
                                lgf.minute   = fix.dateTime.minutes;
                                lgf.second   = fix.dateTime.seconds;
                                gpxLogger.update(lgf);
                            }
                            xSemaphoreGive(gpsMutex);
                        }

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
 *          generation. Runs on core 1 with 11KB stack size. Peak usage measured
 *          at ~33KB with screenshot (scshot); 11KB provides 25% margin. The task
 *          processes commands at 60ms intervals to maintain responsive user interaction.
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
 * @details Creates and starts the CLI task on core 1 with 11KB stack size and priority 1.
 *          Only compiled when CLI functionality is enabled (not DISABLE_CLI).
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 11264, NULL, 1, &cliTaskHandle, 1); }

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

        if (sensors().hasCompass())
        {
            int newHeading = sensors().heading();
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
            if (sensors().hasAmbient())
            {
                AmbientData ambient;
                sensors().readAmbient(ambient);
                if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
                {
                    globalSensorData.temperature = ambient.temperature;
                    globalSensorData.pressure    = ambient.pressure;
                    globalSensorData.humidity    = ambient.humidity;
                    globalSensorData.altitude    = (int16_t)ambient.altitude;
                    xSemaphoreGive(sensorMutex);
                }
            }

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

            bool currentWifiState = connectivity().isConnected();
            if (currentWifiState != sensorState.lastWifiState)
            {
                if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, MUTEX_TIMEOUT_SLOW) == pdTRUE)
                {
                    lv_subject_set_int(&subject_wifi, currentWifiState ? 1 : 0);
                    sensorState.lastWifiState = currentWifiState;
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            float rawBattery = sensors().batteryLevel();
            if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
            {
                globalSensorData.batteryPercent = rawBattery;
                globalSensorData.batteryVoltage = sensors().batteryVoltage();
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
    xTaskCreatePinnedToCore(sensorTask, "Sensor Task", 3072, NULL, 1, &sensorTaskHandle, 0);
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
 * @details Creates and starts the GUI task on core 1 with 9.5KB stack and priority 3.
 *          This ensures that UI updates and touch events are processed with the
 *          highest application priority.
 */
void initGuiTask()
{
    xTaskCreatePinnedToCore(guiTask, "GUI Task", 9728, NULL, 3, &guiTaskHandle, 1);
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

            Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
            TrackVector newRoute;
            RouterResult res = router.route(gpsSnap.latitude, gpsSnap.longitude,
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
                float oldLat = gps.getSnapshot().latitude;
                gps.simFakeGPS(trackData, 40, 500);
                Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
                if (gpsSnap.latitude != oldLat && lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    lv_subject_set_int(&subject_lat,     (int32_t)(gpsSnap.latitude  * 1000000.0f));
                    lv_subject_set_int(&subject_lon,     (int32_t)(gpsSnap.longitude * 1000000.0f));
                    lv_subject_set_int(&subject_heading, (int32_t)gpsSnap.heading);
                    lv_subject_set_int(&subject_speed,   (int32_t)gpsSnap.speed);
                    xSemaphoreGive(lvgl_mutex);
                }
            }

            Gps::GpsSnapshot navSnap = gps.getSnapshot();
            if (navSnap.speed != 0 || navSet.simNavigation)
            {
                unsigned long now = (unsigned long)(esp_timer_get_time() / 1000ULL);
                if (now - lastNavUpdate > 100)
                {
                    lastNavUpdate = now;
                    if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
                    {
                        updateNavigation(navSnap.latitude, navSnap.longitude,
                                         navSnap.heading,  navSnap.speed,
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
 * @details Creates and starts the nav task on core 1 with 3KB stack and priority 1.
 */
void initNavTask()
{
    xTaskCreatePinnedToCore(navTask, "Nav Task", 3072, NULL, 2, &navTaskHandle, 1);
}

/**
 * @brief Suspends all FreeRTOS tasks before entering light sleep.
 *
 * @details Suspends GPS, sensor, navigation and map render tasks in dependency
 *          order to avoid inconsistent state during device suspend.
 */
void suspendAllTasks()
{
    if (gpsTaskHandle != NULL)
        vTaskSuspend(gpsTaskHandle);
    if (sensorTaskHandle != NULL)
        vTaskSuspend(sensorTaskHandle);
    if (navTaskHandle != NULL)
        vTaskSuspend(navTaskHandle);
    TaskHandle_t renderHandle = mapView.renderTaskHandle();
    if (renderHandle != NULL)
        vTaskSuspend(renderHandle);
}

/**
 * @brief Resumes all FreeRTOS tasks after returning from light sleep.
 *
 * @details Resumes tasks in reverse suspension order.
 */
void resumeAllTasks()
{
    TaskHandle_t renderHandle = mapView.renderTaskHandle();
    if (renderHandle != NULL)
        vTaskResume(renderHandle);
    if (navTaskHandle != NULL)
        vTaskResume(navTaskHandle);
    if (sensorTaskHandle != NULL)
        vTaskResume(sensorTaskHandle);
    if (gpsTaskHandle != NULL)
        vTaskResume(gpsTaskHandle);
}
