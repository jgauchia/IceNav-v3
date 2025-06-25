/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.2.3
 * @date 2025-06
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

void gpsTask(void *pvParameters);
void initGpsTask();

#ifndef DISABLE_CLI
	void cliTask(void *param);
	void initCLITask();
#endif
