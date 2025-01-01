/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Waypoint list screen
 * @version 0.2.0_alpha
 * @date 2025-01
 */

#ifndef WAYPOINTLISTSCR_HPP
#define WAYPOINTLISTSCR_HPP

#include "mainScr.hpp"

void loadMainScreen();
void updateWaypointPos();

void waypointListEvent(lv_event_t * event);
void createWaypointListScreen();
void updateWaypointListScreen();

#endif