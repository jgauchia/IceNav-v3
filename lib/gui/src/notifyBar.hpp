/**
 * @file notifyBar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief LVGL - Notify Bar Screen
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "globalGuiDef.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "styles.hpp"

/**
 * @brief Notify Bar screen objects.
 *
 * @details Objects used to display information in the notification bar
 */
extern lv_obj_t *gpsTime;      /**< Time display object. */
extern lv_obj_t *gpsCount;     /**< Satellite count object. */
extern lv_obj_t *gpsFix;       /**< Satellite fix status object. */
extern lv_obj_t *gpsFixMode;   /**< Satellite fix mode object. */
extern lv_obj_t *battIcon;     /**< Battery level icon object. */
extern lv_obj_t *sdCard;       /**< SD card icon object. */
extern lv_obj_t *temp;         /**< Temperature display object. */
extern lv_obj_t *wifi;         /**< WiFi status object. */


/**
 * @brief Temperature values.
 *
 * @details Format string for displaying time in HH:MM:SS format
 */
static const char* timeFormat = "%02d:%02d:%02d";

void createNotifyBar();
