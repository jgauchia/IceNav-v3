/**
 * @file waypointScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Add Waypoint Screen
 * @version 0.2.0_alpha
 * @date 2024-12
 */

#ifndef waypointScr_HPP
#define waypointScr_HPP

#include "addWaypoint.hpp"
#include "mainScr.hpp"

extern lv_obj_t *waypointName;
extern bool isScreenRotated;

void loadMainScreen();

static void waypointScreenEvent(lv_event_t *event);
static void rotateScreen(lv_event_t *event);
static void waypointNameEvent(lv_event_t *event);

void updateWaypointPos();
void createWaypointScreen();

#endif
