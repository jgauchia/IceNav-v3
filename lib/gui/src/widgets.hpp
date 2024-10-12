/**
 * @file widgets.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Widgets
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include "lvgl.h"
#include "globalGuiDef.h"
#include "globalMapsDef.h"
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

void editWidget(lv_event_t *event);
void dragWidget(lv_event_t *event);
void unselectWidget(lv_event_t *event);

static bool widgetSelected = false;
static bool canMoveWidget = false;
static lv_coord_t newX = 0;
static lv_coord_t newY = 0;

static const char *arrowIconFile PROGMEM = "/arrow.bin";    // Compass Arrow Icon
static const char *positionIconFile PROGMEM = "/pin.bin";   // Position Icon
static const char *altitudeIconFile PROGMEM = "/altit.bin"; // Altitude Icon
static const char *speedIconFile PROGMEM = "/speed.bin";    // Speed Icon

void positionWidget(_lv_obj_t *screen);
void compassWidget(_lv_obj_t *screen);
void altitudeWidget(_lv_obj_t *screen);
void speedWidget(_lv_obj_t *screen);

#endif