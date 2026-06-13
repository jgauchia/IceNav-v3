/**
 * @file optionsPanel.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Options Panel
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "globalGuiDef.h"
#include "mainScr.hpp"
#include "gpxDetailScr.hpp"
#include "gpxScr.hpp"

static const char *waypointIconFile = "/gfx/wpt.bin";      /**< Waypoint icon file path. */
static const char *trackIconFile = "/gfx/track.bin";       /**< Track icon file path. */
static const char *settingsIconFile = "/gfx/settings.bin"; /**< Settings icon file path. */
static const char *saveIconFile = "/gfx/save.bin";         /**< Save icon file path. */
static const char *loadIconFile = "/gfx/load.bin";         /**< Load icon file path. */
static const char *editIconFile = "/gfx/edit.bin";         /**< Edit icon file path. */
static const char *deleteIconFile = "/gfx/delete.bin";     /**< Delete icon file path. */
static const char *addWptIconFile = "/gfx/addwpt.bin";     /**< Add Waypoint icon file path. */

extern bool isWaypointOpt;  /**< Waypoint Option selected. */
extern bool isTrackOpt;     /**< Track Option selected. */
extern bool isBarOpen;      /**< Flag to determine if options panel is open. */
extern bool isSubPanelOpen; /**< Flag to determine if sub-panel is open. */

void optionsPanelEvent(lv_event_t *event);
void optionEvent(lv_event_t *event);
void hideShowEvent(lv_event_t * e);
void slideAnim(void * var, int32_t v);
void openOptionsPanel();
void closeOptionsPanel();
void openSubPanel();
void closeSubPanel();
void createOptionsPanel();
