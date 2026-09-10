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

#include <string.h>

#ifdef PANEL_BUS_DSI
    #include <esp_lcd_panel_ops.h>
    #include "esp_heap_caps.h"
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
            // area reflects the logical (post-rotation) dimensions.
            int32_t w = area.x2 - area.x1 + 1;
            int32_t h = area.y2 - area.y1 + 1;
            
            if (w == tft.height() && h == tft.width()) {
                // Rotated 270 degrees (Logical is 800x480, physical is 480x800)
                uint16_t* rotBuf = (uint16_t*)heap_caps_malloc(tft.width() * tft.height() * 2, MALLOC_CAP_SPIRAM);
                if (rotBuf) {
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++) {
                            int dst_x = tft.width() - 1 - y;
                            int dst_y = x;
                            rotBuf[dst_x + dst_y * tft.width()] = pixels[x + y * w];
                        }
                    }
                    esp_lcd_panel_draw_bitmap(static_cast<PANEL_TYPE *>(tft.getPanel())->panelHandle(), 0, 0, tft.width(), tft.height(), rotBuf);
                    tft.waitDisplay();
                    if (_captureBuffer != nullptr)
                        memcpy(_captureBuffer, rotBuf, _captureWidth * _captureHeight * sizeof(uint16_t));
                    heap_caps_free(rotBuf);
                    return;
                }
            }
            if (_captureBuffer != nullptr)
                captureArea(area, pixels);
            esp_lcd_panel_draw_bitmap(static_cast<PANEL_TYPE *>(tft.getPanel())->panelHandle(), 0, 0, tft.width(), tft.height(), pixels);
        #else
            uint32_t w = area.x2 - area.x1 + 1;
            uint32_t h = area.y2 - area.y1 + 1;
            tft.waitDMA();
            tft.waitDisplay();
            if (_captureBuffer != nullptr)
                captureArea(area, pixels);
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

    bool beginCapture(uint16_t *buffer, uint16_t width, uint16_t height) override
    {
        _captureBuffer = buffer;
        _captureWidth = width;
        _captureHeight = height;
        return true;
    }

    void endCapture() override
    {
        _captureBuffer = nullptr;
        _captureWidth = 0;
        _captureHeight = 0;
    }

private:
    void captureArea(const DisplayArea &area, uint16_t *pixels)
    {
        uint32_t w = area.x2 - area.x1 + 1;
        uint32_t h = area.y2 - area.y1 + 1;
        for (uint32_t y = 0; y < h; ++y)
            memcpy(_captureBuffer + (area.y1 + y) * _captureWidth + area.x1, pixels + y * w, w * sizeof(uint16_t));
    }

    uint16_t *_captureBuffer = nullptr;
    uint16_t _captureWidth = 0;
    uint16_t _captureHeight = 0;
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
