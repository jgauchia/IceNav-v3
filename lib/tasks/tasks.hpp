/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef TASKS_HPP
#define TASKS_HPP

#include "gps.hpp"
#include "bme.hpp"
#include "battery.hpp"
#include "compass.hpp"
#include "lvgl.h"
#include "cli.hpp"
#include "mainScr.hpp"
#include "globalMapsDef.h"
#include "globalGpxDef.h"
#include "lvglFuncs.hpp"

#define TASK_SLEEP_PERIOD_MS 5

void gpsTask(void *pvParameters);
void initGpsTask();
void initCLITask();

#endif
