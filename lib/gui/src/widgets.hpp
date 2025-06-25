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
 * Declarations for UI widget objects 
 */
extern lv_obj_t *latitude;         /**< Latitude label */
extern lv_obj_t *longitude;        /**< Longitude label */
extern lv_obj_t *compassHeading;   /**< Compass heading label */
extern lv_obj_t *compassImg;       /**< Compass image object */
extern lv_obj_t *altitude;         /**< Altitude label */
extern lv_obj_t *speedLabel;       /**< Speed label */
extern lv_obj_t *sunriseLabel;     /**< Sunrise time label */
extern lv_obj_t *sunsetLabel;      /**< Sunset time label */
extern lv_obj_t *navArrow;         /**< Navigation arrow object */
extern lv_obj_t *zoomLabel;        /**< Zoom level label */
extern lv_obj_t *zoomWidget;       /**< Zoom widget object */
extern lv_obj_t *mapSpeedLabel;    /**< Map speed label */
extern lv_obj_t *mapSpeed;         /**< Map speed value object */
extern lv_obj_t *miniCompass;      /**< Mini compass widget */
extern lv_obj_t *mapCompassImg;    /**< Map compass image object */
extern lv_obj_t *scaleWidget;      /**< Scale widget object */
extern lv_obj_t *scaleLabel;       /**< Scale label */

void editWidget(lv_event_t *event);
void dragWidget(lv_event_t *event);
void unselectWidget(lv_event_t *event);

static bool widgetSelected = false;      /**< Widget selection state */
static bool canMoveWidget = false;       /**< Permission to move widget */
static lv_coord_t newX = 0;              /**< New X coordinate for widget movement */
static lv_coord_t newY = 0;              /**< New Y coordinate for widget movement */

extern bool isScrolled;                  /**< Flag to indicate when tileview was scrolled */

static const char *arrowIconFile PROGMEM      = "/arrow.bin";       /**< Compass arrow icon */
static const char *positionIconFile PROGMEM   = "/pin.bin";         /**< Position icon */
static const char *altitudeIconFile PROGMEM   = "/altit.bin";       /**< Altitude icon */
static const char *speedIconFile PROGMEM      = "/speed.bin";       /**< Speed icon */
static const char *sunriseIconFile PROGMEM    = "/sunrise.bin";     /**< Sunrise icon */
static const char *sunsetIconFile PROGMEM     = "/sunset.bin";      /**< Sunset icon */
static const char *navArrowIconFile PROGMEM   = "/navarrow.bin";    /**< Navigation arrow icon */
static const char *zoomIconFile PROGMEM       = "/zoom.bin";        /**< Zoom map icon */
static const char *mapSpeedIconFile PROGMEM   = "/mapspeed.bin";    /**< Speed map icon */

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