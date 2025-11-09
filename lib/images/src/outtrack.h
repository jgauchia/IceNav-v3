#pragma once
#include <pgmspace.h>
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_OUTTRACK
#define LV_ATTRIBUTE_IMAGE_OUTTRACK
#endif

extern const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_OUTTRACK uint8_t outtrack_map[] PROGMEM;