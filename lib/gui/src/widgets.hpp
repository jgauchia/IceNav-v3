/**
 * @file widgets.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Widgets
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include "lvglFuncs.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "mapVars.h"

/**
 * @brief Widget objects
 *
 */
extern lv_obj_t *latitude;
extern lv_obj_t *longitude;
extern lv_obj_t *compassHeading;
extern lv_obj_t *compassImg;
extern lv_obj_t *altitude;
extern lv_obj_t *speedLabel;
extern lv_obj_t *sunriseLabel;
extern lv_obj_t *sunsetLabel;
extern lv_obj_t *navArrow;
extern lv_obj_t *zoomLabel;
extern lv_obj_t *zoomWidget;
extern lv_obj_t *mapSpeedLabel;
extern lv_obj_t *mapSpeed;
extern lv_obj_t *miniCompass;
extern lv_obj_t *mapCompassImg;
extern lv_obj_t *scaleWidget;
extern lv_obj_t *scaleLabel;
extern lv_obj_t *turnByTurn;
extern lv_obj_t *turnDistLabel;
extern lv_obj_t *turnImg;

void editWidget(lv_event_t *event);
void dragWidget(lv_event_t *event);
void unselectWidget(lv_event_t *event);

static bool widgetSelected = false;
static bool canMoveWidget = false;
static lv_coord_t newX = 0;
static lv_coord_t newY = 0;

extern bool isScrolled;              // Flag to indicate when tileview was scrolled

static const char *arrowIconFile PROGMEM = "/arrow.bin";            // Compass Arrow Icon
static const char *positionIconFile PROGMEM = "/pin.bin";           // Position Icon
static const char *altitudeIconFile PROGMEM = "/altit.bin";         // Altitude Icon
static const char *speedIconFile PROGMEM = "/speed.bin";            // Speed Icon
static const char *sunriseIconFile PROGMEM = "/sunrise.bin";        // Sunrise Icon
static const char *sunsetIconFile PROGMEM = "/sunset.bin";          // Sunset Icon
static const char *navArrowIconFile PROGMEM = "/navarrow.bin";      // Navigation Arrow icon
static const char *zoomIconFile PROGMEM = "/zoom.bin";              // Zoom map icon
static const char *mapSpeedIconFile PROGMEM = "/mapspeed.bin";      // Speed map icon
static const char *turnStraightIcon PROGMEM = "/STRAIGHT.bin";
static const char *turnSleftIcon PROGMEM = "/SLLEFT.bin";
static const char *turnSrightIcon PROGMEM = "/SLRIGHT.bin";
static const char *turnTleftIcon PROGMEM = "/TLEFT.bin";
static const char *turnTrigthIcon PROGMEM = "/TRIGHT.bin";
static const char *turnUleftIcon PROGMEM = "/ULEFT.bin";
static const char *turnUrigthIcon PROGMEM = "/URIGHT.bin";


void positionWidget(lv_obj_t *screen);
void compassWidget(lv_obj_t *screen);
void altitudeWidget(lv_obj_t *screen);
void speedWidget(lv_obj_t *screen);
void sunWidget(lv_obj_t *screen);
void navArrowWidget(lv_obj_t *screen);
void mapZoomWidget(lv_obj_t *screen);
void mapSpeedWidget(lv_obj_t *screen);
void mapCompassWidget(lv_obj_t *screen);
void mapScaleWidget(lv_obj_t *screen);
void turnByTurnWidget(lv_obj_t *screen);