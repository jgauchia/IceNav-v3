/**
 * @file notifyBar.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief LVGL - Notify Bar Screen
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef NOTIFYBAR_HPP
#define NOTIFYBAR_HPP

#include "globalGuiDef.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "battery.hpp"
#include "settings.hpp"

/**
 * @brief Notify Bar screen objects
 *
 */
static lv_obj_t *gpsTime;    // Time
static lv_obj_t *gpsCount;   // Satellite count
static lv_obj_t *gpsFix;     // Satellite fix
static lv_obj_t *gpsFixMode; // Satellite fix mode
static lv_obj_t *battIcon;    // Battery level
static lv_obj_t *sdCard;     // SD card icon
static lv_obj_t *temp;       // Temperature
static lv_obj_t *wifi;       // Wifi 

static float battLevel = 0;
static float battLevelOld = 0;

#define UPDATE_NOTIFY_PERIOD 1000 // Notify Bar update time

/**
 * @brief Temperature values
 *
 */
static const char* timeFormat PROGMEM = "%02d:%02d:%02d";

void updateNotifyBar(lv_event_t *event);
void updateNotifyBarTimer(lv_timer_t *t);
void createNotifyBar();

#endif
