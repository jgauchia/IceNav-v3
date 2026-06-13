/**
 * @file navarrow.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Navigation arrow icon image descriptor
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_NAVARROW
    #define LV_ATTRIBUTE_IMAGE_NAVARROW
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_NAVARROW uint8_t navarrow_map[];
extern const lv_img_dsc_t navarrow;
