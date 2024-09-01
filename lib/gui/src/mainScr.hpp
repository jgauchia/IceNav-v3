/**
 * @file mainScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Main Screen
 * @version 0.1.8_Alpha
 * @date 2024-08
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
#include "navWpt.hpp"

static lv_timer_t *mainTimer;    // Main Screen Timer
#define UPDATE_MAINSCR_PERIOD 30 // Main Screen update time

extern bool isMainScreen;                          // Flag to indicate main screen is selected
extern bool isReady;                               // Flag to indicate when tileview scroll was finished
static TFT_eSprite zoomSprite = TFT_eSprite(&tft); // Zoom sprite

static const char *arrowIconFile PROGMEM = "F:/arrow.bin";    // Compass Arrow Icon
static const char *positionIconFile PROGMEM = "F:/pin.bin";   // Position Icon
static const char *altitudeIconFile PROGMEM = "F:/altit.bin"; // Altitude Icon
static const char *speedIconFile PROGMEM = "F:/speed.bin";    // Speed Icon

extern uint8_t activeTile; // Active Tile in TileView control
enum tileName
{
  COMPASS,
  MAP,
  NAV,
  SATTRACK,
};

static bool widgetSelected = false;
static bool canMoveWidget = false;

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

/**
 * @brief Compass Tile screen objects
 *
 */
extern lv_obj_t *compassHeading;
extern lv_obj_t *compassImg;
extern lv_obj_t *latitude;
extern lv_obj_t *longitude;
extern lv_obj_t *altitude;
extern lv_obj_t *speedLabel;

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
static lv_obj_t *pdopLabel;
static lv_obj_t *hdopLabel;
static lv_obj_t *vdopLabel;
static lv_obj_t *altLabel;
static lv_style_t styleRadio;
static lv_style_t styleRadioChk;
static uint32_t activeGnss = 0;

void updateCompassScr(lv_event_t * event);

void editScreen(lv_event_t *event);
void unselectWidget(lv_event_t *event);

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
void activeGnssEvent(lv_event_t *event);
void updateSatTrack(lv_event_t *event);
void toolBarEvent(lv_event_t *event);
void fullScreenEvent(lv_event_t *event);
void zoomOutEvent(lv_event_t *event);
void zoomInEvent(lv_event_t *event);

void createMainScr();

#endif
