/**
 * @file input.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 touch input interface
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <stdint.h>

/**
 * @struct TouchPoint
 * @brief Layer-1 touch coordinate, independent of the panel driver.
 */
struct TouchPoint
{
    int16_t x;
    int16_t y;
};

/**
 * @class IInput
 * @brief Layer-1 contract for the touch controller.
 *
 * @details Abstracts reading raw touch points from the panel controller
 *          (FT5x06/XPT2046/GT911...) so the gesture logic in Layer-2 never
 *          talks to LovyanGFX nor the shared I2C bus directly. Gesture
 *          interpretation (drag, pinch, double-tap) stays in the LVGL read
 *          callback; this interface only delivers the raw points.
 */
class IInput
{
public:
    virtual ~IInput() = default;

    /**
     * @brief Read the current touch points from the controller.
     *
     * @param points Destination buffer for the detected points.
     * @param max    Capacity of the destination buffer.
     * @return Number of points written (>= 0), or a negative value if the
     *         controller could not be read this cycle (e.g. shared bus busy),
     *         so the caller can hold the previous state.
     */
    virtual int readTouch(TouchPoint *points, uint8_t max) = 0;
};

IInput &input();
