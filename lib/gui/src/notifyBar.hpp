/**
 * @file notifyBar.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief LVGL - Notify Bar Screen
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef NOTIFYBAR_HPP
#define NOTIFYBAR_HPP

#include <WiFi.h>
#include "globalGuiDef.h"
#include "tasks.hpp"
#include "storage.hpp"

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
static lv_obj_t *wifi;       // Wifi 

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
