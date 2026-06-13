/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen definitions
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "maps.hpp"
#include "globalGuiDef.h"
#include "optionsPanel.hpp"
#include "widgets.hpp"
#include "navScr.hpp"
#include "satInfoScr.hpp"
#include "nmeaDebugScr.hpp"
#include "compass.hpp"
#include "navigation.hpp"

extern bool isMainScreen;        /**< Flag to indicate main screen is selected */
extern bool canScrollMap;        /**< Flag to indicate if map can be scrolled */
extern bool isScrollingMap;      /**< Flag to indicate if map is scrolling */

extern uint8_t activeTile;       /**< Active Tile in TileView control */

/**
 * @brief Enum for identifying different tile screens in the application.
 */
enum tileName
{
    COMPASS,    /**< Compass screen (Tile 0) */
    MAP,        /**< Map screen (Tile 1) */
    NAV,        /**< Navigation screen (Tile 2) */
    SATTRACK,   /**< Satellite track screen (Tile 3) */
    DEBUG_NMEA, /**< NMEA debug screen (Tile 4) — temporary */
};

/**
 * @brief Main Screen Tiles
 * @details LVGL tile objects for main application screens.
 */
extern lv_obj_t *compassTile;     /**< Compass screen tile */
extern lv_obj_t *navTile;         /**< Navigation screen tile */
extern lv_obj_t *mapTile;         /**< Map screen tile */
extern lv_obj_t *satTrackTile;    /**< Satellite track screen tile */
extern lv_obj_t *nmeaDebugTile;   /**< NMEA debug tile (temporary) */

/**
 * @brief Map Toolbar Buttons
 * @details Toolbar button objects and toolbar layout configuration.
 */
extern lv_obj_t *btnZoomIn;       /**< Toolbar button for zooming in */
extern lv_obj_t *btnZoomOut;      /**< Toolbar button for zooming out */
extern lv_obj_t *btnToggle3D;     /**< Toggle 3D/2D map view button */
extern uint8_t toolBarOffset;     /**< Offset for toolbar positioning */
extern uint8_t toolBarSpace;      /**< Space between toolbar buttons */

void triggerMapRedraw();
void createMainScr();