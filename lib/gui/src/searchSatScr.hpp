/**
 * @file searchSatScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.2.5
 * @date 2026-04
 */

#pragma once

#include "gps.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *searchSatScreen;                                    /**< Search Satellite Screen  */
extern lv_timer_t *searchTimer;                                      /**< Timer for satellite search process */
static const char* textSearch = "Searching for satellites";  /**< Search status message  */
static const char* satIconFile = "/sat.bin";                 /**< Path to satellite icon file */
static const char *skipIconFile = "/skip.bin";               /**< Path to skip icon file */
static const char *confIconFile = "/settings.bin";           /**< Path to settings icon file */

void loadMainScreen();
void searchGPS(lv_timer_t *searchTimer);
void buttonEvent(lv_event_t *event);
void createSearchSatScr();
