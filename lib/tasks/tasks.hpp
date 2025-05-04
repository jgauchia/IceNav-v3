/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#ifndef TASKS_HPP
#define TASKS_HPP

#include "gps.hpp"
#include "bme.hpp"
#include "battery.hpp"
#include "compass.hpp"
#include "lvgl.h"
#include "cli.hpp"
//#include "mainScr.hpp"
#include "globalGpxDef.h"
#include "lvglFuncs.hpp"

#define TASK_SLEEP_PERIOD_MS 5

void gpsTask(void *pvParameters);
void initGpsTask();

#ifndef DISABLE_CLI
    void cliTask(void *param);
    void initCLITask();
#endif

#endif
