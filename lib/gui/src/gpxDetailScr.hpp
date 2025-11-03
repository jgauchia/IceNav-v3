/**
 * @file gpxDetailScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX Tag detail Screen
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include "gpxFiles.hpp"
#include "gpxScr.hpp"
#include "mainScr.hpp"

extern lv_obj_t *gpxTagValue;      /**< Add/Edit Waypoint name screen text area */
extern lv_obj_t *gpxTag;           /**< Name Tag label  */
extern lv_obj_t *labelLat;         /**< Latitude label */
extern lv_obj_t *labelLatValue;    /**< Latitude value label */
extern lv_obj_t *labelLon;         /**< Longitude label */
extern lv_obj_t *labelLonValue;    /**< Longitude value label */
extern bool isScreenRotated;       /**< Flag to know if screen is rotated */

void loadMainScreen();

static void gpxDetailScreenEvent(lv_event_t *event);
static void rotateScreen(lv_event_t *event);
static void gpxTagNameEvent(lv_event_t *event);

void updateWaypoint();
void createGpxDetailScreen();