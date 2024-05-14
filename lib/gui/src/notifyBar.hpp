/**
 * @file notifyBar.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief LVGL - Notify Bar Screen
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef NOTIFYBAR_HPP
#define NOTIFYBAR_HPP

#include <TimeLib.h>
#include "globalGuiDef.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "battery.hpp"
#include "bme.hpp"

/**
 * @brief Notify Bar screen objects
 *
 */
static lv_obj_t *gpsTime;    // Time
static lv_obj_t *gpsCount;   // Satellite count
static lv_obj_t *gpsFix;     // Satellite fix
static lv_obj_t *gpsFixMode; // Satellite fix mode
static lv_obj_t *battery;    // Battery level
static lv_obj_t *sdCard;     // SD card icon
static lv_obj_t *temp;       // Temperature

#define UPDATE_NOTIFY_PERIOD 1000 // Notify Bar update time

/**
 * @brief Battery values
 *
 */
static uint8_t battLevel = 0;
static uint8_t battLevelOld = 0;

/**
 * @brief Temperature values
 *
 */
static uint8_t tempOld = 0;
static const char* timeFormat PROGMEM = "%02d:%02d:%02d";

void updateBatt(lv_event_t *event);
void updateFixMode(lv_event_t *event);
void updateTime(lv_event_t *event);
void updateGpsCount(lv_event_t *event);

void updateNotifyBar(lv_timer_t *t);
void createNotifyBar();

#endif