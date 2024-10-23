/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Main Screen
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef MAINSCR_HPP
#define MAINSCR_HPP

#include "globalGuiDef.h"
#include "lvglFuncs.hpp"
#include "misc/lv_color.h"
#include "satInfo.hpp"
#include "notifyBar.hpp"
#include "buttonBar.hpp"
#include "renderMaps.hpp"
#include "vectorMaps.hpp"
#include "addWaypoint.hpp"
#include "loadWaypoint.hpp"
#include "deleteWaypoint.hpp"
#include "editWaypoint.hpp"
#include "widgets.hpp"
#include "navScr.hpp"
#include "satInfoScr.hpp"

static lv_timer_t *mainTimer;    // Main Screen Timer
#define UPDATE_MAINSCR_PERIOD 30 // Main Screen update time

extern bool isMainScreen;                          // Flag to indicate main screen is selected
extern bool isReady;                               // Flag to indicate when tileview scroll was finished
static TFT_eSprite zoomSprite = TFT_eSprite(&tft); // Zoom sprite

extern uint8_t activeTile; // Active Tile in TileView control

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
extern int toolBarOffset;
extern int toolBarSpace;

void updateCompassScr(lv_event_t * event);

void deleteMapScrSprites();
void createMapScrSprites();

void getActTile(lv_event_t *event);
void scrollTile(lv_event_t *event);

void generateRenderMap();
void generateVectorMap();
void updateMainScreen(lv_timer_t *t);
void gestureEvent(lv_event_t *event);
void deleteMapScrSprites();
void createMapScrSprites();
void drawMapWidgets();
void updateMap(lv_event_t *event);
void updateSatTrack(lv_event_t *event);
void toolBarEvent(lv_event_t *event);
void fullScreenEvent(lv_event_t *event);
void zoomOutEvent(lv_event_t *event);
void zoomInEvent(lv_event_t *event);
void updateNavEvent(lv_event_t *event);

void createMainScr();

#endif
