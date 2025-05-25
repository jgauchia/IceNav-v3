/**
 * @file gpxDetailScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX Tag detail Screen
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include "gpxFiles.hpp"
#include "gpxScr.hpp"
#include "mainScr.hpp"

extern lv_obj_t *gpxTagValue;
extern lv_obj_t *gpxTag;
extern lv_obj_t *labelLat;
extern lv_obj_t *labelLatValue;
extern lv_obj_t *labelLon;
extern lv_obj_t *labelLonValue;
extern bool isScreenRotated;

void loadMainScreen();

static void gpxDetailScreenEvent(lv_event_t *event);
static void rotateScreen(lv_event_t *event);
static void gpxTagNameEvent(lv_event_t *event);

void updateWaypoint();
void createGpxDetailScreen();