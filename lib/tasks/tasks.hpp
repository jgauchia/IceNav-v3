/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

#ifndef TASKS_HPP
#define TASKS_HPP

#include <Timezone.h>
#include <TimeLib.h>
#include "gps.hpp"
#include "bme.hpp"
#include "battery.hpp"
#include "compass.hpp"
#include "lvgl.h"
#include "cli.hpp"
#include "navWpt.hpp"
#include "mainScr.hpp"
#include "globalMapsDef.h"
#include "globalGpxDef.h"
#include "lvglFuncs.hpp"

#define TASK_SLEEP_PERIOD_MS 5

/**
 * @brief Central European Time (daylight saving time)
 *
 */
static TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
static TimeChangeRule CET  = {"CET ", Last, Sun, Oct, 3, 60};
static Timezone CE(CEST,CET);

/**
 * @brief Time Variables
 * 
 */
extern time_t local, utc;

void gpsTask(void *pvParameters);
void initGpsTask();
void initCLITask();

#endif
