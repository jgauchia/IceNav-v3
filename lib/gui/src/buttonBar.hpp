/**
 * @file buttonBar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Button Bar
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include "globalGuiDef.h"
#include "mainScr.hpp"
#include "gpxDetailScr.hpp"
#include "gpxScr.hpp"

static const char *waypointIconFile PROGMEM = "/wpt.bin";      /**< Waypoint icon file path. */
static const char *trackIconFile PROGMEM = "/track.bin";       /**< Track icon file path. */
static const char *settingsIconFile PROGMEM = "/settings.bin"; /**< Settings icon file path. */
static const char *saveIconFile PROGMEM = "/save.bin";         /**< Save icon file path. */
static const char *loadIconFile PROGMEM = "/load.bin";         /**< Load icon file path. */
static const char *editIconFile PROGMEM = "/edit.bin";         /**< Edit icon file path. */
static const char *deleteIconFile PROGMEM = "/delete.bin";     /**< Delete icon file path. */
static const char *menuIconFile PROGMEM = "/menu.bin";         /**< Menu icon file path. */
static const char *addWptIconFile PROGMEM = "/addwpt.bin";     /**< Add Waypoint icon file path. */
static const char *exitIconFile PROGMEM = "/exit.bin";         /**< Exit icon file path. */

static lv_obj_t *option; /**< Pointer to the currently selected option object (LVGL). */

extern bool isWaypointOpt;  /**< Waypoint Option selected. */
extern bool isTrackOpt;     /**< Track Option selected. */
extern bool isOptionLoaded; /**< Option selected & loaded. */
extern bool isBarOpen;      /**< Flag to determine if Button Bar is open. */


void buttonBarEvent(lv_event_t *event);
void optionEvent(lv_event_t *event);
void hideShowEvent(lv_event_t * e);
void hideShowAnim(void * var, int32_t v);
void createButtonBarScr();
void loadOptions();
