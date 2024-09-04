/**
 * @file navWpt.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Waypoint Navigation Functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

#ifndef NAVWPT_HPP
#define NAVWPT_HPP

#include "lvgl.h"
#include "addWaypoint.hpp"
#include "gpsMath.hpp"

char *latFormatString(double lat);
char *lonFormatString(double lon);

/**
 * @brief Navigation Tile screen objects
 *
 */
 extern lv_obj_t *nameNav;
 extern lv_obj_t *latNav;
 extern lv_obj_t *lonNav;
 extern lv_obj_t *distNav;
 extern lv_obj_t *arrowNav;

void updateNavScreen();

#endif