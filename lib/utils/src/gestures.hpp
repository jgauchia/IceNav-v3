/**
 * @file gestures.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include <LovyanGFX.hpp>
#include "tft.hpp"

#define TOUCH_MAX_POINTS 2                  /**< Maximum number of supported touch points */
#define TOUCH_DOUBLE_TOUCH_INTERVAL 150     /**< Interval (ms) to detect double touch */
#define SPEED_FAST 1.0f                     /**< Fast gesture speed threshold (px/ms) */
#define SPEED_SLOW 0.3f                     /**< Slow gesture speed threshold (px/ms) */

static bool countTouchReleases = false;         /**< Indicates if touch release events are being counted */
static int numberTouchReleases = 0;             /**< Number of detected touch releases */
static uint32_t firstTouchReleaseTime = 0;      /**< Timestamp of the first touch release event */

/**
 * @brief Zoom gesture direction enumeration
 * 
 */
typedef enum 
{
  ZOOM_NONE = 0,    /**< No zoom gesture detected */
  ZOOM_IN,          /**< Pinch out gesture detected (zoom in) */
  ZOOM_OUT          /**< Pinch in gesture detected (zoom out) */
} zoom_dir;

zoom_dir pinchZoom(const lgfx::touch_point_t prev[TOUCH_MAX_POINTS],const lgfx::touch_point_t curr[TOUCH_MAX_POINTS],float dt_ms);