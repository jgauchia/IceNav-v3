/**
 * @file mapSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Map Settings screen
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef MAPSETTINGSCR_HPP
#define MAPSETTINGSCR_HPP

#include "globalGuiDef.h"
#include "settings.hpp"
#include "lvglFuncs.hpp"
#include "vectorMaps.hpp"
#include "renderMaps.hpp"

static lv_obj_t *mapSettingsOptions;
static lv_obj_t *mapSwitch;
static lv_obj_t *mapType;
static lv_obj_t *zoomLevel;

static void mapSettingsBack(lv_event_t *event);
static void configureMapType(lv_event_t *event);
static void configureMapRotation(lv_event_t *event);
static void incrementZoom(lv_event_t *event);
static void decrementZoom(lv_event_t *event);
static void showCompass(lv_event_t *event);
static void showSpeed(lv_event_t *event);
static void showScale(lv_event_t *event);

void createMapSettingsScr();

#endif