/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef TASKS_HPP
#define TASKS_HPP

#include <Timezone.h>
#include <TimeLib.h>
#include "gps.hpp"
#include "bme.hpp"
#include "battery.hpp"
#include "lvgl.h"

#define TASK_SLEEP_PERIOD_MS 4

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

/**
 * @brief Temperature reading values 
 * 
 */
extern uint8_t tempValue;
extern uint8_t tempOld;


/**
 * @brief Battery values
 *
 */
extern uint8_t battLevel;
extern uint8_t battLevelOld;

void sensorsTask(void *pvParameters);
void lvglTask(void *pvParameters);
void initTasks();

#endif