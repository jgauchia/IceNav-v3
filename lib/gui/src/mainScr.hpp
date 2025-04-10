/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.1.9
 * @date 2025-04
 */

#ifndef MAINSCR_HPP
#define MAINSCR_HPP

#include "maps.hpp"

#include "globalGuiDef.h"
#include "buttonBar.hpp"
#include "widgets.hpp"
#include "navScr.hpp"
#include "satInfoScr.hpp"
#include "compass.hpp"

extern lv_timer_t *mainTimer;    // Main Screen Timer
#define UPDATE_MAINSCR_PERIOD 30 // Main Screen update time

extern bool isScrolled;                            // Flag to indicate when tileview was scrolled
extern bool isMainScreen;                          // Flag to indicate main screen is selected
extern bool isReady;                               // Flag to indicate when tileview scroll was finished
static TFT_eSprite zoomSprite = TFT_eSprite(&tft); // Zoom sprite

extern uint8_t activeTile; // Active Tile in TileView control
extern int heading;        // Heading value (Compass or GPS)

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
void toolBarEvent(lv_event_t *event);
void fullScreenEvent(lv_event_t *event);
void zoomOutEvent(lv_event_t *event);
void zoomInEvent(lv_event_t *event);
void updateNavEvent(lv_event_t *event);

void createMainScr();

#endif
