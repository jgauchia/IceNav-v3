/**
 * @file searchSatScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.1.8
 * @date 2024-04
 */

#ifndef SEARCHSATSCR_HPP
#define SEARCHSATSCR_HPP

#include <lvgl.h>
#include "gps.hpp"

extern lv_obj_t *searchSatScreen;     // Search Satellite Screen
static lv_timer_t *searchTimer;
static const char* textSearch PROGMEM = "Searching for satellites";
static const char* satIconFile PROGMEM = "F:/sat.bin";

void loadMainScreen();
void searchGPS(lv_timer_t *searchTimer);
void createSearchSatScr();

#endif