/**
 * @file inputP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 touch input implementation (LovyanGFX panel controller)
 * @version 0.3.0
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "input.hpp"
#include "tft.hpp"
#include "i2c_espidf.hpp"

/**
 * @class InputP4
 * @brief Layer-0 touch implementation for ESP32-P4 boards over LovyanGFX.
 *
 * @details For the 3.5" board the FT6336 shares the I2C bus, so access is guarded
 *          by the bus lock exactly as on the S3 shared-bus panels; when the bus is
 *          busy the read is reported as failed so the caller holds the previous
 *          state. The 4.3" multitouch controller is added in a later phase.
 */
class InputP4 : public IInput
{
public:
    int readTouch(TouchPoint *points, uint8_t max) override
    {
        lgfx::touch_point_t raw[MAX_RAW_POINTS];
        uint8_t toRead = (max < MAX_RAW_POINTS) ? max : MAX_RAW_POINTS;
        int count = 0;

        #ifdef WAVESHARE_P4_35
            // Protect I2C bus access for FT6336 on shared bus.
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
 * @brief Provides the P4 input implementation as the Layer-1 singleton.
 */
IInput &input()
{
    static InputP4 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32P4
