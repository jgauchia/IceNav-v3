/**
 * @file widgets.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Widgets
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "lvglFuncs.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "mapVars.h"
#include "styles.hpp"

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
extern lv_obj_t *turnByTurn;       /**< Turn-by-Turn navigation widget*/
extern lv_obj_t *turnDistLabel;    /**< Label object showing turn distance */
extern lv_obj_t *turnImg;          /**< Image object for turn indication */
extern lv_obj_t *climbOverlay;  /**< climb overlay widget */
extern lv_obj_t *climbDistLabel;   /**< Remaining climb distance label */
extern lv_obj_t *climbGainLabel;   /**< Remaining elevation gain label */
extern lv_obj_t *climbGradeLabel;  /**< Current grade label */
extern lv_obj_t *climbCanvas;          /**< Elevation profile canvas */
extern lv_obj_t *climbSegLabel;        /**< "N/M" segment counter label */
extern lv_obj_t *climbCatLabel;        /**< "CAT 2" / "HC" category label */
extern lv_obj_t *climbTotalDistLabel;  /**< Total segment distance label */
extern lv_obj_t *climbTotalGainLabel;  /**< Total segment elevation gain label */
extern lv_obj_t *climbAvgGradeLabel;   /**< Average grade of segment label */

void editWidget(lv_event_t *event);
void dragWidget(lv_event_t *event);
void unselectWidget(lv_event_t *event);

static bool widgetSelected = false;      /**< Widget selection state */
static lv_coord_t newX = 0;              /**< New X coordinate for widget movement */
static lv_coord_t newY = 0;              /**< New Y coordinate for widget movement */

extern bool isScrolled;                  /**< Flag to indicate when tileview was scrolled */

static const char *toggle3DIconFile   = "/gfx/2dview.bin";      /**< 3D map mode toggle icon */
static const char *toggle2DIconFile   = "/gfx/3dview.bin";      /**< 2D map mode toggle icon */


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
void climbWidget(lv_obj_t *screen);