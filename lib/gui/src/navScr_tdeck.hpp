/**
 * @file navScr_tdeck.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Navigation screen for lilygo T-DECK
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef NAVSCR_TDECK_HPP
#define NAVSCR_TDECK_HPP

#include "lvgl.h"
#include "globalGuiDef.h"
#include "navup.c"

/**
 * @brief Navigation Tile screen objects
 *
 */
extern lv_obj_t *nameNav;
extern lv_obj_t *latNav;
extern lv_obj_t *lonNav;
extern lv_obj_t *distNav;
extern lv_obj_t *arrowNav;
extern char* destName;

void navigationScr(_lv_obj_t *screen);

#endif