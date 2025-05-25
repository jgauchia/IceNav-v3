/**
 * @file searchSatScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include "gps.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *searchSatScreen;     // Search Satellite Screen
static lv_timer_t *searchTimer;
static const char* textSearch PROGMEM = "Searching for satellites";
static const char* satIconFile PROGMEM = "/sat.bin";
static const char *skipIconFile PROGMEM = "/skip.bin";     // Skip icon
static const char *confIconFile PROGMEM = "/settings.bin"; // Settings icon

void loadMainScreen();
void searchGPS(lv_timer_t *searchTimer);
void buttonEvent(lv_event_t *event);
void createSearchSatScr();
