/**
 * @file displayS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 display implementation (LovyanGFX)
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32S3

#include "display.hpp"
#include "mapCanvas.hpp"
#include "tft.hpp"
#include "panelSelect.hpp"

#include <string.h>

#ifdef T4_S3
#include "LILYGO_T4_S3.hpp"
#endif

/**
 * @class DisplayS3
 * @brief Layer-0 display implementation for ESP32-S3 boards over LovyanGFX.
 *
 * @details Wraps the existing global tft object and its helpers. The flush path
 *          starts the DMA transfer and returns; LVGL waits for the previous
 *          transfer through waitFlushDone() before reusing a draw buffer,
 *          overlapping rasterisation with the in-flight DMA.
 */
class DisplayS3 : public IDisplay
{
public:
    void init() override
    {
        initTFT();
    }

    uint16_t width() const override
    {
        return tft.width();
    }

    uint16_t height() const override
    {
        return tft.height();
    }

    void setBrightness(uint8_t value) override
    {
        tft.setBrightness(value);
    }

    void setRotation(uint8_t rotation) override
    {
        tft.setRotation(rotation);
    }

    void clear(uint32_t color) override
    {
        tft.fillScreen(color);
    }

    void sleepOn(uint8_t brightness) override
    {
        tftOn(brightness);
    }

    void sleepOff() override
    {
        tftOff();
    }

    void flush(const DisplayArea &area, uint16_t *pixels) override
    {
        uint32_t w = area.x2 - area.x1 + 1;
        uint32_t h = area.y2 - area.y1 + 1;
        tft.waitDMA();
        if (_captureBuffer != nullptr)
        {
            for (uint32_t y = 0; y < h; ++y)
                memcpy(_captureBuffer + (area.y1 + y) * _captureWidth + area.x1, pixels + y * w, w * sizeof(uint16_t));
        }
        #ifdef T4_S3
            tft.setAddrWindow(area.x1, area.y1, w, h);
            tft.pushPixelsDMA(pixels, w * h, true);
        #else
            tft.setSwapBytes(true);
            tft.setAddrWindow(area.x1, area.y1, w, h);
            tft.pushImageDMA(area.x1, area.y1, w, h, pixels);
            tft.setSwapBytes(false);
        #endif
    }

    void waitFlushDone() override
    {
        tft.waitDMA();
    }

    bool beginCapture(uint16_t *buffer, uint16_t width, uint16_t height) override
    {
        (void)height;
        _captureBuffer = buffer;
        _captureWidth = width;
        return true;
    }

    void endCapture() override
    {
        _captureBuffer = nullptr;
        _captureWidth = 0;
    }

private:
    uint16_t *_captureBuffer = nullptr;
    uint16_t _captureWidth = 0;
};

/**
 * @brief Provides the S3 display implementation as the Layer-1 singleton.
 */
IDisplay &display()
{
    static DisplayS3 instance;
    return instance;
}

/**
 * @brief Returns the global LovyanGFX device map canvases attach to.
 */
LovyanGFX *mapCanvasParent()
{
    return &tft;
}

#endif // CONFIG_IDF_TARGET_ESP32S3
