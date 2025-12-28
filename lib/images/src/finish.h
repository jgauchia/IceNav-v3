#pragma once
#include <pgmspace.h>
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_FINISH
    #define LV_ATTRIBUTE_IMAGE_FINISH
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_FINISH uint8_t finish_map[] PROGMEM;