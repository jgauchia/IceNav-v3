/**
 * @file display.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 display interface
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <stdint.h>

struct DisplayArea
{
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
};

/**
 * @class IDisplay
 * @brief Layer-1 contract for low-frequency display operations.
 *
 * @details Covers init, geometry, brightness and the LVGL flush path with DMA
 *          synchronisation. The high-frequency drawing surface used by the map
 *          renderer is MapCanvas (see mapCanvas.hpp), kept out of this virtual
 *          interface to avoid indirection in the rasterisation hot path.
 */
class IDisplay
{
public:
    virtual ~IDisplay() = default;
    virtual void init() = 0;
    virtual uint16_t width() const = 0;
    virtual uint16_t height() const = 0;
    virtual void setBrightness(uint8_t value) = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual void clear(uint32_t color) = 0;
    virtual void sleepOn(uint8_t brightness) = 0;
    virtual void sleepOff() = 0;
    virtual void flush(const DisplayArea &area, uint16_t *pixels) = 0;
    virtual void waitFlushDone() = 0;

    virtual bool beginCapture(uint16_t *buffer, uint16_t width, uint16_t height)
    {
        (void)buffer;
        (void)width;
        (void)height;
        return false;
    }

    virtual void endCapture()
    {
    }
};

IDisplay &display();
