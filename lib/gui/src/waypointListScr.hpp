/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Waypoint list screen
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef WAYPOINTLISTSCR_HPP
#define WAYPOINTLISTSCR_HPP

#include "mainScr.hpp"
#include "storage.hpp"
#include "gpxParser.hpp"

void loadMainScreen();
void updateWaypointPos();

void waypointListEvent(lv_event_t * event);
void createWaypointListScreen();
void updateWaypointListScreen();

#endif