/**
 * @file addWaypointScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Add Waypoint Screen
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef ADDWAYPOINTSCR_HPP
#define ADDWAYPOINTSCR_HPP

#include "globalGuiDef.h"
#include "mainScr.hpp"

extern lv_obj_t *waypointName;
extern bool isScreenRotated;

void loadMainScreen();

static void addWaypointEvent(lv_event_t *event);
static void rotateScreen(lv_event_t *event);

void updateWaypointPos();
void createAddWaypointScreen();

#endif
