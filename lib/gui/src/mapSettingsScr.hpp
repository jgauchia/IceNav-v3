/**
 * @file mapSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Map Settings screen
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "globalGuiDef.h"
#include "lvglFuncs.hpp"
#include "settings.hpp"
#include "maps.hpp"

/**
 * @brief Screen state tracking structure for performance optimization.
 *
 * @details Stores previous values of screen data to prevent unnecessary LVGL updates.
 * Implements dirty flag pattern to only redraw when values actually change.
 */
struct ScreenState 
{
    int lastHeading = -1;
    int16_t lastAltitude = -32768;
    float lastLat = NAN;
    float lastLon = NAN;
    float lastSpeed = -1;
    bool needsRedraw = true;
};

extern struct ScreenState screenState;

static lv_obj_t *mapSettingsOptions;   /**< Map settings options container object. */
static lv_obj_t *mapSwitch;            /**< Map enable/disable switch object. */
static lv_obj_t *mapType;              /**< Map type selection object. */
static lv_obj_t *zoomLevel;            /**< Zoom level selection object. */
static lv_obj_t *btnBack;              /**< Back button object in map settings. */
static lv_obj_t *zoomBtnUp;            /**< Zoom level increment button object. */
static lv_obj_t *zoomBtnDown;          /**< Zoom level decrement button object. */
static lv_obj_t *list;                 /**< List object for settings options. */
static lv_obj_t *checkCompass;         /**< Checkbox for enabling compass display. */
static lv_obj_t *checkCompassRot;      /**< Checkbox for enabling compass rotation. */
static lv_obj_t *checkSpeed;           /**< Checkbox for displaying speed on the map. */
static lv_obj_t *checkScale;           /**< Checkbox for displaying map scale. */
static lv_obj_t *checkFullScreen;      /**< Checkbox for enabling fullscreen map display. */


static void mapSettingsEvents(lv_event_t *event);
void createMapSettingsScr();
