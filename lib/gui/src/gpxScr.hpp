/**
 * @file gpxScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX list screen
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#ifndef GPXSCR_HPP
#define GPXSCR_HPP

#include "mainScr.hpp"
#include "storage.hpp"
#include "gpxParser.hpp"
#include "buttonBar.hpp"

void loadMainScreen();
void updateWaypointPos();

void gpxListEvent(lv_event_t * event);
void createGpxListScreen();
void updateGpxListScreen();

#endif