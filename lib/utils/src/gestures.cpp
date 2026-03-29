/**
 * @file gestures.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.4
 * @date 2025-12
 */

#include "gestures.hpp"

/**
 * @brief Detects movement for pinch-zoom with automatic dynamic threshold
 *
 * @param prev Previous touch points.
 * @param curr Current touch points.
 * @param dt_ms Time delta in ms.
 * @return zoom_dir enum.
 */
 zoom_dir pinchZoom(const lgfx::touch_point_t prev[TOUCH_MAX_POINTS],const lgfx::touch_point_t curr[TOUCH_MAX_POINTS],float dt_ms)
 {
    float distPrev = hypotf(prev[0].x - prev[1].x, prev[0].y - prev[1].y);
    float distCurr = hypotf(curr[0].x - curr[1].x, curr[0].y - curr[1].y);
    float delta = distCurr - distPrev;
    float speed = (dt_ms > 0.0f) ? fabsf(delta) / dt_ms : 0.0f;
    float diag = hypotf(tft.width(), tft.height()); // calculated once
    float thresholdIn  = 0.03f  * diag;
    float thresholdOut = 0.012f * diag;
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
