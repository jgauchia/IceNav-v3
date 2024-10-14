/**
 * @file mapSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Map Settings screen
 * @version 0.1.8_Alpha
 * @date 2024-10
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
static lv_obj_t *btnBack;
static lv_obj_t *zoomBtnUp;
static lv_obj_t *zoomBtnDown;
static lv_obj_t *list;
static lv_obj_t *checkCompass;
static lv_obj_t *checkCompassRot;
static lv_obj_t *checkSpeed;
static lv_obj_t *checkScale;
static lv_obj_t *checkFullScreen;

static void mapSettingsEvents(lv_event_t *event);
void createMapSettingsScr();

#endif
