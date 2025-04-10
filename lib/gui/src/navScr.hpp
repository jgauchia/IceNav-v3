/**
 * @file navScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Navigation screen 
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef NAVSCR_HPP
#define NAVSCR_HPP

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

void navigationScr(_lv_obj_t *screen);

#endif