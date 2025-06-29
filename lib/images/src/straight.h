#pragma once
#include <pgmspace.h>
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_STRAIGHT
#define LV_ATTRIBUTE_IMAGE_STRAIGHT
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_STRAIGHT uint8_t straight_map[] PROGMEM;