/**
 * @file buttonBar.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Button Bar
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef BUTTONBAR_HPP
#define BUTTONBAR_HPP

#include "globalGuiDef.h"
#include "mainScr.hpp"

static const char *waypointIconFile PROGMEM = "F:/wpt.bin";      // Waypoint icon
static const char *trackIconFile PROGMEM = "F:/track.bin";       // Track icon
static const char *settingsIconFile PROGMEM = "F:/settings.bin"; // Settings icon
static const char *saveIconFile PROGMEM = "F:/save.bin";         // Save icon
static const char *loadIconFile PROGMEM = "F:/load.bin";         // Load icon
static const char *deleteIconFile PROGMEM = "F:/delete.bin";     // Delete icon
static const char *menuIconFile PROGMEM = "F/menu.bin";          // Menu icon
static const char *addWptIconFile PROGMEM = "F:/addwpt.bin";         // Add Waypoint icon

static lv_obj_t *option;

extern bool isWaypointOpt;  // Waypoint Option selected
extern bool isTrackOpt;     // Track Option selected
extern bool isOptionLoaded; // Option selected & loaded

void buttonBarEvent(lv_event_t *event);
void optionEvent(lv_event_t *event);
void hideShowEvent(lv_event_t * e);
void hideShowAnim(void * var, int32_t v);
void startHideShowAnim(lv_anim_t * anim);
void endHideShowAnim(lv_anim_t *anim);
void createButtonBarScr();
void loadOptions();

#endif
