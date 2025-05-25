/**
 * @file gpxScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX list screen
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include "mainScr.hpp"
#include "storage.hpp"
#include "gpxParser.hpp"
#include "buttonBar.hpp"

extern String gpxFileFolder;
extern bool gpxTrack;
extern bool gpxWaypoint;

void loadMainScreen();
void updateWaypoint(uint8_t action);

void gpxListEvent(lv_event_t * event);
void createGpxListScreen();
void updateGpxListScreen();