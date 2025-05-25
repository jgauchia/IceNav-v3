/**
 * @file gestures.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include <LovyanGFX.hpp>
#include "tft.hpp"

#define TOUCH_MAX_POINTS 2
#define TOUCH_DOUBLE_TOUCH_INTERVAL 150
#define SPEED_FAST 1.0f   // px/ms fast speed
#define SPEED_SLOW 0.3f   // px/ms slow speed

static bool countTouchReleases = false;
static int numberTouchReleases = 0;
static uint32_t firstTouchReleaseTime = 0;

typedef enum 
{
  ZOOM_NONE = 0,
  ZOOM_IN,
  ZOOM_OUT
} zoom_dir;

zoom_dir pinchZoom(const lgfx::touch_point_t prev[TOUCH_MAX_POINTS],const lgfx::touch_point_t curr[TOUCH_MAX_POINTS],float dt_ms);