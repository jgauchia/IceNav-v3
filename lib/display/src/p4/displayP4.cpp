/**
 * @file displayP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 display implementation (LovyanGFX)
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "display.hpp"
#include "mapCanvas.hpp"
#include "tft.hpp"
#include "panelSelect.hpp"
#include "esp_log.h"

#ifdef PANEL_BUS_DSI
    #include <esp_lcd_panel_ops.h>
#endif

/**
 * @class DisplayP4
 * @brief Layer-0 display implementation for ESP32-P4 boards over LovyanGFX.
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
        #ifdef PANEL_BUS_DSI
            // LVGL render mode FULL guarantees one flush per frame with the
            // full-screen draw buffer — no shadow copy needed.
            esp_lcd_panel_draw_bitmap(static_cast<PANEL_TYPE *>(tft.getPanel())->panelHandle(), 0, 0, TFT_WIDTH, TFT_HEIGHT, pixels);
        #else
            uint32_t w = area.x2 - area.x1 + 1;
            uint32_t h = area.y2 - area.y1 + 1;
            tft.waitDMA();
            tft.waitDisplay();
            tft.setSwapBytes(true);
            tft.setAddrWindow(area.x1, area.y1, w, h);
            tft.pushImageDMA(area.x1, area.y1, w, h, pixels);
            tft.setSwapBytes(false);
        #endif
    }

    void waitFlushDone() override
    {
        #ifdef PANEL_BUS_DSI
            tft.waitDisplay();
        #else
            tft.waitDMA();
        #endif
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
