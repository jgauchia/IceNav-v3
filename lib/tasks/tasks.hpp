/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks header definitions for GPS and CLI management
 * @version 0.2.9
 * @date 2026-06
 * @details This header defines the interface for FreeRTOS tasks used for GPS data processing
 *          and CLI interface management. It provides function declarations and configuration
 *          constants for task management.
 */

#pragma once

#include "gps.hpp"
#include "bme.hpp"
#include "battery.hpp"
#include "compass.hpp"
#include "lvgl.h"
#include "cli.hpp"
#include "globalGpxDef.h"
#include "lvglFuncs.hpp"

/**
 * @struct SensorData
 * @brief Holds the latest synchronized data from all non-GPS sensors.
 */
struct SensorData
{
    float batteryPercent  = 0.0f;
    float batteryVoltage  = 0.0f;
    int16_t altitude      = 0;
    int heading           = 0;
    float temperature     = 0.0f;
    float pressure        = 0.0f;
    float humidity        = 0.0f;
    float accelX          = 0.0f;
    float accelY          = 0.0f;
    float accelZ          = 0.0f;
    float gyroX           = 0.0f;
    float gyroY           = 0.0f;
    float gyroZ           = 0.0f;
};

extern SensorData globalSensorData;
extern SemaphoreHandle_t sensorMutex;

// NMEA debug stats — written by gpsTask, read by debug tile
extern uint32_t nmeaDebugOk;
extern uint32_t nmeaDebugErr;
extern uint32_t nmeaDebugCycles;
extern uint8_t  nmeaLastMsg;
extern portMUX_TYPE nmeaDebugMux;

// Raw NMEA sentence ring buffer — written by gpsTask, read by debug tile.
// Captures each complete sentence as it is fed to the parser, without
// disabling parsing. Protected by nmeaDebugMux.
#define NMEA_RAW_LINES 12   /**< Number of sentences kept in the ring buffer. */
#define NMEA_RAW_LEN   88   /**< Max bytes stored per sentence (incl. terminator). */
extern char    nmeaRawBuf[NMEA_RAW_LINES][NMEA_RAW_LEN];
extern uint8_t nmeaRawHead;  /**< Index of the next slot to write (oldest line). */

extern TaskHandle_t gpsTaskHandle;
extern TaskHandle_t guiTaskHandle;

void gpsTask(void *pvParameters);

void initGpsTask();

void sensorTask(void *pvParameters);

void initSensorTask();

void guiTask(void *pvParameters);

void initGuiTask();

#ifndef DISABLE_CLI
    void cliTask(void *param);

    void initCLITask();
#endif

void navTask(void *pvParameters);

void initNavTask();
