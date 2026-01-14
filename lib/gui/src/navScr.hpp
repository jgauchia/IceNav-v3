/**
 * @file navScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Navigation screen 
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "globalGuiDef.h"
#include "../../images/src/navup.h"

/**
 * @brief Navigation Tile screen objects.
 *
 * @details References to objects used in the navigation tile for displaying waypoint name, latitude, longitude, distance, and navigation arrow.
 */
extern lv_obj_t *nameNav;    /**< Navigation waypoint name label. */
extern lv_obj_t *latNav;     /**< Navigation latitude label. */
extern lv_obj_t *lonNav;     /**< Navigation longitude label. */
extern lv_obj_t *distNav;    /**< Navigation distance label. */
extern lv_obj_t *arrowNav;   /**< Navigation arrow image object. */

void navigationScr(_lv_obj_t *screen);