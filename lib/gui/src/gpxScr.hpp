/**
 * @file gpxScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX list screen
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include "mainScr.hpp"
#include "storage.hpp"
#include "gpxParser.hpp"
#include "buttonBar.hpp"

extern String gpxFileFolder;  /**< GPX file folder path */
extern bool gpxTrack;         /**< Track mode flag */
extern bool gpxWaypoint;      /**< Waypoint mode flag */

void loadMainScreen();
void updateWaypoint(uint8_t action);

void gpxListEvent(lv_event_t * event);
void createGpxListScreen();
void updateGpxListScreen();