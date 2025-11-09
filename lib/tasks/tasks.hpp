/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks header definitions for GPS and CLI management
 * @version 0.2.3
 * @date 2025-11
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
 * @brief GPS data processing task function
 * @param pvParameters Task parameters (unused)
 */
void gpsTask(void *pvParameters);

/**
 * @brief Initialize GPS processing task
 */
void initGpsTask();

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
