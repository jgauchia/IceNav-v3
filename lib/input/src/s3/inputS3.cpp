/**
 * @file inputS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 touch input implementation (LovyanGFX panel controller)
 * @version 0.3.0
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32S3

#include "input.hpp"
#include "tft.hpp"

/**
 * @class InputS3
 * @brief Layer-0 touch implementation for ESP32-S3 boards over LovyanGFX.
 *
 * @details Reads raw points from the panel touch controller directly via
 *          LovyanGFX. On boards with an I2C touch panel (TOUCH_CAPACITIVE),
 *          all I2C sensors share the same lgfx::i2c-owned bus (see
 *          I2CDriverBase::beginShared()), so there is a single logical bus
 *          owner and no separate lock is needed here.
 */
class InputS3 : public IInput
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
 * @brief Provides the S3 input implementation as the Layer-1 singleton.
 */
IInput &input()
{
    static InputS3 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32S3
