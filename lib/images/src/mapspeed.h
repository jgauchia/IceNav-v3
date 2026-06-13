/**
 * @file mapspeed.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Map speed icon image descriptor
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_MAPSPEED
    #define LV_ATTRIBUTE_IMAGE_MAPSPEED
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_MAPSPEED uint8_t mapspeed_map[];
extern const lv_img_dsc_t mapspeed;
