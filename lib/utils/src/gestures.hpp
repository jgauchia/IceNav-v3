/**
 * @file gestures.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#pragma once

#include <LovyanGFX.hpp>
#include "tft.hpp"

#define TOUCH_MAX_POINTS 2
#define SPEED_FAST 1.0f   // px/ms fast speed
#define SPEED_SLOW 0.3f   // px/ms slow speed

typedef enum 
{
  ZOOM_NONE = 0,
  ZOOM_IN,
  ZOOM_OUT
} zoom_dir;

zoom_dir pinchZoom(const lgfx::touch_point_t prev[TOUCH_MAX_POINTS],const lgfx::touch_point_t curr[TOUCH_MAX_POINTS],float dt_ms);