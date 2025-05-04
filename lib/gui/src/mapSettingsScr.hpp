/**
 * @file mapSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Map Settings screen
 * @version 0.2.0_alpha
 * @date 2025-01
 */

#ifndef MAPSETTINGSCR_HPP
#define MAPSETTINGSCR_HPP

#include "globalGuiDef.h"
#include "lvglFuncs.hpp"
#include "settings.hpp"
#include "maps.hpp"

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
