/**
 * @file inputP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 touch input implementation (LovyanGFX)
 * @version 0.3.0
 * @date 2026-07
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "input.hpp"
#include "tft.hpp"

/**
 * @class InputP4
 * @brief Layer-0 touch implementation for ESP32-P4 boards over LovyanGFX.
 */
class InputP4 : public IInput
{
public:
    int readTouch(TouchPoint *points, uint8_t max) override
    {
        lgfx::touch_point_t raw[MAX_RAW_POINTS];
        uint8_t toRead = (max < MAX_RAW_POINTS) ? max : MAX_RAW_POINTS;

        int count = tft.getTouch(raw, toRead);

        for (int i = 0; i < count && i < toRead; i++)
        {
            points[i].x = raw[i].x;
            points[i].y = raw[i].y;
        }

        return count;
    }

private:
    static constexpr uint8_t MAX_RAW_POINTS = 2;
};

/**
 * @brief Provides the P4 input implementation as the Layer-1 singleton.
 */
IInput &input()
{
    static InputP4 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32P4
