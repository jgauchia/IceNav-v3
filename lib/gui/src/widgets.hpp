/**
 * @file widgets.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Widgets
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include "lvglFuncs.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"

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

void editWidget(lv_event_t *event);
void dragWidget(lv_event_t *event);
void unselectWidget(lv_event_t *event);

static bool widgetSelected = false;
static bool canMoveWidget = false;
static lv_coord_t newX = 0;
static lv_coord_t newY = 0;

extern bool isScrolled;              // Flag to indicate when tileview was scrolled

static const char *arrowIconFile PROGMEM = "/arrow.bin";     // Compass Arrow Icon
static const char *positionIconFile PROGMEM = "/pin.bin";    // Position Icon
static const char *altitudeIconFile PROGMEM = "/altit.bin";  // Altitude Icon
static const char *speedIconFile PROGMEM = "/speed.bin";     // Speed Icon
static const char *sunriseIconFile PROGMEM = "/sunrise.bin"; // Sunrise Icon
static const char *sunsetIconFile PROGMEM = "/sunset.bin";   // Sunset Icon

void positionWidget(_lv_obj_t *screen);
void compassWidget(_lv_obj_t *screen);
void altitudeWidget(_lv_obj_t *screen);
void speedWidget(_lv_obj_t *screen);
void sunWidget(_lv_obj_t *screen);

#endif