/**
 * @file notifyBar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief LVGL - Notify Bar Screen
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include "globalGuiDef.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "battery.hpp"
#include "settings.hpp"

/**
 * @brief Notify Bar screen objects.
 *
 * @details Objects used to display information in the notification bar
 */
static lv_obj_t *gpsTime;      /**< Time display object. */
static lv_obj_t *gpsCount;     /**< Satellite count object. */
static lv_obj_t *gpsFix;       /**< Satellite fix status object. */
static lv_obj_t *gpsFixMode;   /**< Satellite fix mode object. */
static lv_obj_t *battIcon;     /**< Battery level icon object. */
static lv_obj_t *sdCard;       /**< SD card icon object. */
static lv_obj_t *temp;         /**< Temperature display object. */
static lv_obj_t *wifi;         /**< WiFi status object. */

static float battLevel = 0;      /**< Current battery level value. */
static float battLevelOld = 0;   /**< Previous battery level value. */

#define UPDATE_NOTIFY_PERIOD 1000 /**< Notify Bar update time in milliseconds. */

/**
 * @brief Temperature values.
 *
 * @details Format string for displaying time in HH:MM:SS format
 */
static const char* timeFormat PROGMEM = "%02d:%02d:%02d";

void updateNotifyBar(lv_event_t *event);
void updateNotifyBarTimer(lv_timer_t *t);
void createNotifyBar();
