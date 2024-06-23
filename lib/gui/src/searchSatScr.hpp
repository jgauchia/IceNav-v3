/**
 * @file searchSatScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef SEARCHSATSCR_HPP
#define SEARCHSATSCR_HPP

#include <lvgl.h>
#include "gps.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *searchSatScreen;     // Search Satellite Screen
static lv_timer_t *searchTimer;
static const char* textSearch PROGMEM = "Searching for satellites";
static const char* satIconFile PROGMEM = "F:/sat.bin";
static const char *skipIconFile PROGMEM = "F:/skip.bin";     // Skip icon
static const char *confIconFile PROGMEM = "F:/settings.bin"; // Settings icon

void loadMainScreen();
void searchGPS(lv_timer_t *searchTimer);
void buttonEvent(lv_event_t *event);
void createSearchSatScr();

#endif
