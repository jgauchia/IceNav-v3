/**
 * @file pin.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Position pin icon image descriptor
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_PIN
    #define LV_ATTRIBUTE_IMAGE_PIN
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_PIN uint8_t pin_map[];
extern const lv_img_dsc_t pin;
