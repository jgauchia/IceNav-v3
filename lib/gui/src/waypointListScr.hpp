/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Waypoint list screen
 * @version 0.1.9_alpha
 * @date 2024-11
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