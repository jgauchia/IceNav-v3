/**
 * @file displayP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 display implementation (LovyanGFX, SPI panel)
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "display.hpp"
#include "mapCanvas.hpp"
#include "tft.hpp"
#include "panelSelect.hpp"

/**
 * @class DisplayP4
 * @brief Layer-0 display implementation for ESP32-P4 boards over LovyanGFX.
 *
 * @details For the 3.5" board the panel is driven over SPI, so the S3 LovyanGFX
 *          path applies unchanged. The MIPI-DSI variant of the 4.3" board is
 *          added in a later phase behind an esp_lcd backend.
 */
class DisplayP4 : public IDisplay
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
        tft.setSwapBytes(true);
        tft.setAddrWindow(area.x1, area.y1, w, h);
        tft.pushImageDMA(area.x1, area.y1, w, h, pixels);
        tft.setSwapBytes(false);
    }

    void waitFlushDone() override
    {
        tft.waitDMA();
    }
};

/**
 * @brief Provides the P4 display implementation as the Layer-1 singleton.
 */
IDisplay &display()
{
    static DisplayP4 instance;
    return instance;
}

/**
 * @brief Returns the global LovyanGFX device map canvases attach to.
 */
LovyanGFX *mapCanvasParent()
{
    return &tft;
}

#endif // CONFIG_IDF_TARGET_ESP32P4
