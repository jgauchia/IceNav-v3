#pragma once
#include <pgmspace.h>
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_SLLEFT
    #define LV_ATTRIBUTE_IMAGE_SLLEFT
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_SLLEFT uint8_t slleft_map[] PROGMEM;