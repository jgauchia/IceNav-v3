/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Waypoint list screen
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef WAYPOINTLISTSCR_HPP
#define WAYPOINTLISTSCR_HPP

#include "globalGuiDef.h"
#include "globalGpxDef.h"
#include "mainScr.hpp"
#include "storage.hpp"
#include <regex>

void loadMainScreen();
void updateWaypointPos();

void waypointListEvent(lv_event_t * event);
void createWaypointListScreen();
void updateWaypointListScreen();

#endif