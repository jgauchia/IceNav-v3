/**
 * @file searchSatScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.3.0
 * @date 2026-10
 */

#pragma once

#include "gps.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *searchSatScreen;                                    /**< Search Satellite Screen  */
static const char* textSearch = "Searching for satellites";  /**< Search status message  */
static const char* satIconFile = "/gfx/sat.bin";                 /**< Path to satellite icon file */
static const char *skipIconFile = "/gfx/skip.bin";               /**< Path to skip icon file */
static const char *confIconFile = "/gfx/settings.bin";           /**< Path to settings icon file */

void loadMainScreen();
void resumeSatSearch();
void searchGPS(lv_timer_t *timer);
void buttonEvent(lv_event_t *event);
void createSearchSatScr();
