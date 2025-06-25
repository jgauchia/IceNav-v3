/**
 * @file gestures.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "gestures.hpp"

/**
 * @brief Detects movement for pinch-zoom with automatic dynamic threshold according to speed
 *
 * Calculates the distance between two touch points in the previous and current states,
 * dynamically adjusts the zoom thresholds based on gesture speed, and determines
 * if a pinch-zoom (in or out) gesture has occurred.
 *
 * @param prev Array of two previous touch points (lgfx::touch_point_t).
 * @param curr Array of two current touch points (lgfx::touch_point_t).
 * @param dt_ms Time in milliseconds between previous and current touch points.
 * @return zoom_dir: ZOOM_NONE, ZOOM_IN, or ZOOM_OUT.
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