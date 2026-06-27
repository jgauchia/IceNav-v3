/**
 * @file inputS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 touch input implementation (LovyanGFX panel controller)
 * @version 0.3.0
 * @date 2026-06
 */

#include "input.hpp"
#include "tft.hpp"
#include "i2c_espidf.hpp"

/**
 * @class InputS3
 * @brief Layer-0 touch implementation for ESP32-S3 boards over LovyanGFX.
 *
 * @details Reads raw points from the panel touch controller. On ICENAV_BOARD
 *          the FT5x06 shares the I2C bus, so access is guarded by the bus lock;
 *          when the bus is busy the read is reported as failed so the caller
 *          holds the previous state instead of glitching the gesture timers.
 */
class InputS3 : public IInput
{
public:
    int readTouch(TouchPoint *points, uint8_t max) override
    {
        lgfx::touch_point_t raw[MAX_RAW_POINTS];
        uint8_t toRead = (max < MAX_RAW_POINTS) ? max : MAX_RAW_POINTS;
        int count = 0;

        #ifdef ICENAV_BOARD
            // Protect I2C bus access for FT5x06 on shared bus.
            if (i2c.lock(0))
            {
                count = tft.getTouch(raw, toRead);
                i2c.unlock();
            }
            else
            {
                return -1;
            }
        #else
            count = tft.getTouch(raw, toRead);
        #endif

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
