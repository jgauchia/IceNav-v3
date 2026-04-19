/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen definitions
 * @version 0.1.9
 * @date 2026-04
 */

#pragma once

#include "maps.hpp"
#include "globalGuiDef.h"
#include "buttonBar.hpp"
#include "widgets.hpp"
#include "navScr.hpp"
#include "satInfoScr.hpp"
#include "compass.hpp"
#include "navigation.hpp"

extern bool isScrolled;          /**< Flag to indicate when tileview was scrolled */
extern bool isMainScreen;        /**< Flag to indicate main screen is selected */
extern bool canScrollMap;        /**< Flag to indicate if map can be scrolled */
extern bool isScrollingMap;      /**< Flag to indicate if map is scrolling */

extern uint8_t activeTile;       /**< Active Tile in TileView control */

static const char *zoomInIconFile = "/zoomin.bin";   /**< Zoom in icon file path */
static const char *zoomOutIconFile = "/zoomout.bin"; /**< Zoom out icon file path */

/**
 * @brief Enum for identifying different tile screens in the application.
 */
enum tileName
{
    COMPASS,   /**< Compass screen (Tile 0) */
    MAP,       /**< Map screen (Tile 1) */
    NAV,       /**< Navigation screen (Tile 2) */
    SATTRACK,  /**< Satellite track screen (Tile 3) */
};

/**
 * @brief Main Screen Tiles
 * @details LVGL tile objects for main application screens.
 */
extern lv_obj_t *compassTile;    /**< Compass screen tile */
extern lv_obj_t *navTile;        /**< Navigation screen tile */
extern lv_obj_t *mapTile;        /**< Map screen tile */
extern lv_obj_t *satTrackTile;   /**< Satellite track screen tile */

/**
 * @brief Map Toolbar Buttons
 * @details Toolbar button objects and toolbar layout configuration.
 */
extern lv_obj_t *btnZoomIn;       /**< Toolbar button for zooming in */
extern lv_obj_t *btnZoomOut;      /**< Toolbar button for zooming out */
extern uint8_t toolBarOffset;     /**< Offset for toolbar positioning */
extern uint8_t toolBarSpace;      /**< Space between toolbar buttons */

void updateCompassScr(lv_event_t * event);
void getActTile(lv_event_t *event);
void scrollTile(lv_event_t *event);
void updateMap(lv_event_t *event);
void mapToolBarEvent(lv_event_t *event);
void scrollMapEvent(lv_event_t *event);
void zoomEvent(lv_event_t *event);
void updateNavEvent(lv_event_t *event);
void createMapImage(_lv_obj_t *screen);
void showMapWidgets();
void hideMapWidgets();
void createMainScr();