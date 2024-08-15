/**
 * @file addWaypointScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Add Waypoint Screen
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef ADDWAYPOINTSCR_HPP
#define ADDWAYPOINTSCR_HPP

#include "globalGuiDef.h"
#include "mainScr.hpp"

extern lv_obj_t *waypointName;

void loadMainScreen();
static void addWaypointEvent(lv_event_t *event);
void createAddWaypointScreen();

#endif
