/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks header definitions for GPS and CLI management
 * @version 0.2.4
 * @date 2025-12
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

#define TASK_SLEEP_PERIOD_MS 5 /**< Sleep period for tasks in milliseconds */

/**
 * @struct SensorData
 * @brief Holds the latest synchronized data from all non-GPS sensors.
 */
struct SensorData {
    float batteryPercent = 0.0f;
    int16_t altitude = 0;
    int heading = 0;
    float temperature = 0.0f;
    float pressure = 0.0f;
    float humidity = 0.0f;
};

extern SensorData globalSensorData;

void gpsTask(void *pvParameters);

void initGpsTask();

void sensorTask(void *pvParameters);

void initSensorTask();

#ifndef DISABLE_CLI
    /**
     * @brief CLI processing task function
     * @param param Task parameters (unused)
     */
    void cliTask(void *param);
    
    /**
     * @brief Initialize CLI processing task
     */
    void initCLITask();
#endif
