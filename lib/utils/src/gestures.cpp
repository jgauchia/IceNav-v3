/**
 * @file gestures.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Touch gestures functions
 * @version 0.2.9
 * @date 2026-06
 */

#include "gestures.hpp"
#include "display.hpp"
#include <cmath>

/**
 * @brief Detects movement for pinch-zoom with automatic dynamic threshold
 *
 * @param prev Previous touch points.
 * @param curr Current touch points.
 * @param dt_ms Time delta in ms.
 * @return zoom_dir enum.
 */
 zoom_dir pinchZoom(const TouchPoint prev[TOUCH_MAX_POINTS],const TouchPoint curr[TOUCH_MAX_POINTS],float dt_ms)
 {
    float distPrev = hypotf(prev[0].x - prev[1].x, prev[0].y - prev[1].y);
    float distCurr = hypotf(curr[0].x - curr[1].x, curr[0].y - curr[1].y);
    float delta = distCurr - distPrev;
    float speed = (dt_ms > 0.0f) ? fabsf(delta) / dt_ms : 0.0f;
    static const float diag = hypotf(display().width(), display().height());
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

/**
 * @brief Detects rotation angle delta from a two-finger twist gesture.
 *
 * @details Computes the angular change of the vector between two touch points
 *          across consecutive samples. Reorders the current touch indices via
 *          nearest-neighbor matching against the previous sample, preventing
 *          spurious 180 degree flips when the controller swaps finger IDs.
 *          Wraps the delta into [-180, 180] and discards values below
 *          ROTATE_ANGLE_THRESHOLD to filter out jitter.
 *
 * @param prev Previous touch points.
 * @param curr Current touch points.
 * @return Rotation delta in degrees (signed). Positive: counter-clockwise. 0 when below threshold.
 */
float pinchRotate(const TouchPoint prev[TOUCH_MAX_POINTS], const TouchPoint curr[TOUCH_MAX_POINTS])
{
    float d00 = hypotf((float)(curr[0].x - prev[0].x), (float)(curr[0].y - prev[0].y));
    float d11 = hypotf((float)(curr[1].x - prev[1].x), (float)(curr[1].y - prev[1].y));
    float d01 = hypotf((float)(curr[0].x - prev[1].x), (float)(curr[0].y - prev[1].y));
    float d10 = hypotf((float)(curr[1].x - prev[0].x), (float)(curr[1].y - prev[0].y));
    int a = 0;
    int b = 1;
    if (d01 + d10 < d00 + d11)
    {
        a = 1;
        b = 0;
    }
    float anglePrev = atan2f((float)(prev[1].y - prev[0].y), (float)(prev[1].x - prev[0].x));
    float angleCurr = atan2f((float)(curr[b].y - curr[a].y), (float)(curr[b].x - curr[a].x));
    float delta = (angleCurr - anglePrev) * (180.0f / (float)M_PI);
    if (delta >  180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    if (fabsf(delta) < ROTATE_ANGLE_THRESHOLD)
        return 0.0f;
    return delta;
}
