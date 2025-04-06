/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Waypoint list screen
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#ifndef WAYPOINTLISTSCR_HPP
#define WAYPOINTLISTSCR_HPP

#include "mainScr.hpp"
#include "gpxParser.hpp"

void loadMainScreen();
void updateWaypointPos();

void waypointListEvent(lv_event_t * event);
void createWaypointListScreen();
void updateWaypointListScreen();

#endif