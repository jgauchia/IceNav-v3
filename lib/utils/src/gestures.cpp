/**
 * @file gestures.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.1
 * @date 2025-05
 */

#include "gestures.hpp"

/**
 * @brief Detects movement for pinch-zoom with automatic dynamic threshold accordint to speed
 *
 * @param prev Array of two previous touch points (lgfx::touch_point_t).
 * @param curr Array de two actual touch points (lgfx::touch_point_t).
 * @param dt_ms time (in ms) between previous and actual touch points.
 * @return zoom_dir: ZOOM_NONE, ZOOM_IN, o ZOOM_OUT.
 */
 zoom_dir pinchZoom(const lgfx::touch_point_t prev[TOUCH_MAX_POINTS],const lgfx::touch_point_t curr[TOUCH_MAX_POINTS],float dt_ms)
 {
  float distPrev = sqrtf((prev[0].x - prev[1].x) * (prev[0].x - prev[1].x) +
                          (prev[0].y - prev[1].y) * (prev[0].y - prev[1].y));
  float distCurr = sqrtf((curr[0].x - curr[1].x) * (curr[0].x - curr[1].x) +
                          (curr[0].y - curr[1].y) * (curr[0].y - curr[1].y));
  float delta = distCurr - distPrev;

  float speed = (dt_ms > 0) ? fabsf(delta) / dt_ms : 0.0f;
  float thresholdIn  = 0.03f * sqrtf(tft.width() * tft.width() + tft.height() * tft.height());  
  float thresholdOut = 0.012f * sqrtf(tft.width() * tft.width() + tft.height() * tft.height()); 

  if (speed > SPEED_FAST) 
  {
    thresholdIn  *= 1.2f; 
    thresholdOut *= 0.7f;
  }
  else if (speed < SPEED_SLOW) 
  {
    thresholdIn  *= 0.7f; 
    thresholdOut *= 0.5f;
  }

  if (delta > thresholdOut) 
    return ZOOM_IN;  
  else if (delta < -thresholdIn) 
    return ZOOM_OUT;   

  return ZOOM_NONE;
}