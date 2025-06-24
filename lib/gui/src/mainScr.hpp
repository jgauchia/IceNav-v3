/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.1.9
 * @date 2025-06
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

extern lv_timer_t *mainTimer;    // Main Screen Timer
#define UPDATE_MAINSCR_PERIOD 30 // Main Screen update time

extern bool isScrolled;                            // Flag to indicate when tileview was scrolled
extern bool isMainScreen;                          // Flag to indicate main screen is selected
extern bool isReady;                               // Flag to indicate when tileview scroll was finished
extern bool canScrollMap;                          // Flag to indicate whet can scroll map
extern bool isScrollingMap;                        // Flag to indicate if map is scrolling
// static TFT_eSprite zoomSprite = TFT_eSprite(&tft); // Zoom sprite

extern uint8_t activeTile; // Active Tile in TileView control
extern int heading;        // Heading value (Compass or GPS)

static const char *zoomInIconFile PROGMEM = "/zoomin.bin";           // Zoom in icon
static const char *zoomOutIconFile PROGMEM = "/zoomout.bin";         // Zoom out icon

enum tileName
{
  COMPASS,
  MAP,
  NAV,
  SATTRACK,
};

/**
 * @brief Main Screen Tiles
 *
 */
extern lv_obj_t *compassTile;
extern lv_obj_t *navTile;
extern lv_obj_t *mapTile;
extern lv_obj_t *satTrackTile;

/**
 * @brief Map Toolbar Buttons
 *
 */
extern lv_obj_t *btnFullScreen;
extern lv_obj_t *btnZoomIn;
extern lv_obj_t *btnZoomOut;
extern uint8_t toolBarOffset;
extern uint8_t toolBarSpace;

void updateCompassScr(lv_event_t * event);

void getActTile(lv_event_t *event);
void scrollTile(lv_event_t *event);

void updateMainScreen(lv_timer_t *t);
void gestureEvent(lv_event_t *event);

void updateMap(lv_event_t *event);
void updateSatTrack(lv_event_t *event);
void mapToolBarEvent(lv_event_t *event);
void scrollMapEvent(lv_event_t *event);
void zoomEvent(lv_event_t *event);
void updateNavEvent(lv_event_t *event);
void createMapCanvas(_lv_obj_t *screen);
void showMapWidgets();
void hideMapWidgets();
void createMainScr();