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
static const char *TAG = "PERF";
#ifdef PANEL_BUS_DSI
    #include <esp_lcd_panel_ops.h>
    #include <esp_heap_caps.h>
    #include <cstring>

    // Avoids pulling in lvgl.h here: lib/display isn't in the LVGL include
    // chain, and this file only needs the display handle and one query.
    extern "C"
    {
        typedef struct _lv_display_t lv_display_t;
        bool lv_display_flush_is_last(lv_display_t *disp);
    }
    extern lv_display_t *display_drv;
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
        uint32_t start = millis();
        #ifdef PANEL_BUS_DSI
            // Partial writes race the continuous DSI scanout (tearing):
            // accumulate areas in a shadow buffer, blit full frames on vsync.
            if (!_shadowFb)
                _shadowFb = (uint16_t *)heap_caps_aligned_alloc(64, (size_t)TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
            if (!_shadowFb)
                return;

            uint32_t w = area.x2 - area.x1 + 1;
            uint32_t h = area.y2 - area.y1 + 1;
            uint16_t *dst = _shadowFb + area.y1 * TFT_WIDTH + area.x1;
            const uint16_t *src = pixels;
            for (uint32_t y = 0; y < h; y++)
            {
                memcpy(dst, src, w * sizeof(uint16_t));
                dst += TFT_WIDTH;
                src += w;
            }

            if (lv_display_flush_is_last(display_drv))
            {
                tft.waitDisplay();
                esp_lcd_panel_draw_bitmap(static_cast<PANEL_TYPE *>(tft.getPanel())->panelHandle(), 0, 0, TFT_WIDTH, TFT_HEIGHT, _shadowFb);
            }
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
        ESP_LOGI(TAG, "flush %lu ms", millis() - start);
    }

    void waitFlushDone() override
    {
        #ifndef PANEL_BUS_DSI
            tft.waitDMA();
        #endif
    }

private:
    #ifdef PANEL_BUS_DSI
        uint16_t *_shadowFb = nullptr;
    #endif
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
