/**
 * @file satInfoScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Satellite info screen 
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef SATINFOSCR_HPP
#define SATINFOSCR_HPP

#include "lvgl.h"
#include "globalGuiDef.h"
#include "gps.hpp"
#include "satInfo.hpp"

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
extern lv_obj_t *pdopLabel;
extern lv_obj_t *hdopLabel;
extern lv_obj_t *vdopLabel;
extern lv_obj_t *altLabel;

void satelliteBar1DrawEvent(lv_event_t * event);
void satelliteScr(_lv_obj_t *screen);

#endif