/**
 * @file Panel_ST7701_DSI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LovyanGFX ST7701 panel driver over MIPI-DSI for ESP32-P4 (Waveshare 4.3")
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Init sequence from the Waveshare BSP (vendor_specific_init_default).
namespace lgfx
{
    inline namespace v1
    {
        struct Panel_ST7701_DSI : public Panel_DSI
        {
        public:
            // MIPI-DSI needs the bus fully up before the panel reset pulse;
            // Panel_Device::init() toggles reset mid bus bring-up.
            bool init(bool use_reset) override
            {
                if (_lines_buffer != nullptr)
                    return false;

                auto bus = getBusDSI();
                if (bus == nullptr)
                    return false;
                if (!bus->init())
                    return false;

                if (_light)
                    _light->init(0);

                if (use_reset && _cfg.pin_rst >= 0)
                {
                    init_rst();
                    rst_control(false);
                    lgfx::delay(10);
                    rst_control(true);
                    lgfx::delay(10);
                }

                bool result = init_dpi(bus) && init_panel();
                if (result)
                {
                    esp_lcd_dpi_panel_get_frame_buffer(_disp_panel_handle, 1, &(_config_detail.buffer));
                    result = build_line_array();
                }

                if (result)
                {
                    if (!_refresh_done_sem)
                        _refresh_done_sem = xSemaphoreCreateBinary();
                    if (_refresh_done_sem)
                    {
                        esp_lcd_dpi_panel_event_callbacks_t callbacks = {};
                        callbacks.on_refresh_done = on_refresh_done;
                        esp_lcd_dpi_panel_register_event_callbacks(_disp_panel_handle, &callbacks, this);
                    }
                }

                return result;
            }

            void waitDisplay(void) override
            {
                if (_refresh_done_sem)
                    xSemaphoreTake(_refresh_done_sem, pdMS_TO_TICKS(100));
            }

            esp_lcd_panel_handle_t panelHandle(void) const { return _disp_panel_handle; }

        protected:
            SemaphoreHandle_t _refresh_done_sem = nullptr;

            static bool IRAM_ATTR on_refresh_done(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
            {
                auto self = static_cast<Panel_ST7701_DSI*>(user_ctx);
                BaseType_t need_yield = pdFALSE;
                xSemaphoreGiveFromISR(self->_refresh_done_sem, &need_yield);
                return need_yield == pdTRUE;
            }

            bool build_line_array(void)
            {
                auto ptr = (uint8_t*)_config_detail.buffer;
                if (ptr == nullptr)
                    return false;

                const auto height = _cfg.panel_height;
                size_t la_size = height * sizeof(void*);
                uint8_t** lineArray = (uint8_t**)heap_alloc_dma(la_size);
                if (nullptr == lineArray)
                    return false;
                memset(lineArray, 0, la_size);

                const size_t line_length = ((_cfg.panel_width * _write_bits >> 3) + 3) & ~3;

                _lines_buffer = lineArray;
                for (int y = 0; y < height; y++)
                {
                    lineArray[y] = ptr;
                    ptr += line_length;
                }
                return true;
            }

            // Entry format: { 1+N, cmd, N data bytes... }
            const uint8_t *getInitParams(size_t listno) const override
            {
                static constexpr const uint8_t list0[] =
                {
                     6, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13,
                     2, 0xEF, 0x08,
                     6, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10,
                     3, 0xC0, 0x63, 0x00,
                     3, 0xC1, 0x0D, 0x02,
                     3, 0xC2, 0x17, 0x08,
                     2, 0xCC, 0x10,
                    17, 0xB0, 0x40, 0xC9, 0x94, 0x0E, 0x10, 0x05, 0x0B, 0x09,
                              0x08, 0x26, 0x04, 0x52, 0x10, 0x69, 0x6B, 0x69,
                    17, 0xB1, 0x40, 0xD2, 0x98, 0x0C, 0x92, 0x07, 0x09, 0x08,
                              0x07, 0x25, 0x02, 0x0E, 0x0C, 0x6E, 0x78, 0x55,

                     6, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11,
                     2, 0xB0, 0x5D,
                     2, 0xB1, 0x4E,
                     2, 0xB2, 0x87,
                     2, 0xB3, 0x80,
                     2, 0xB5, 0x4E,
                     2, 0xB7, 0x85,
                     2, 0xB8, 0x21,
                     3, 0xB9, 0x10, 0x1F,
                     2, 0xBB, 0x03,
                     2, 0xBC, 0x00,
                     2, 0xC1, 0x78,
                     2, 0xC2, 0x78,
                     2, 0xD0, 0x88,
                     4, 0xE0, 0x00, 0x3A, 0x02,
                    12, 0xE1, 0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0,
                              0x00, 0x40, 0x40,
                    14, 0xE2, 0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0,
                              0x00, 0xA0, 0x00, 0xA0, 0x00,
                     5, 0xE3, 0x00, 0x00, 0x33, 0x33,
                     3, 0xE4, 0x44, 0x44,
                    17, 0xE5, 0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0,
                              0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0,
                     5, 0xE6, 0x00, 0x00, 0x33, 0x33,
                     3, 0xE7, 0x44, 0x44,
                    17, 0xE8, 0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0,
                              0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0,
                     8, 0xEB, 0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00,
                     3, 0xEC, 0x08, 0x01,
                    17, 0xED, 0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF,
                              0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B,
                     7, 0xEF, 0x08, 0x08, 0x08, 0x45, 0x3F, 0x54,
                     6, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00,
                     1, 0x11,
                     0,
                };

                static constexpr const uint8_t list1[] =
                {
                     1, 0x29,
                     0,
                };

                switch (listno)
                {
                    case 0: return list0;
                    case 1: return list1;
                    default: return nullptr;
                }
            }

            size_t getInitDelay(size_t listno) const override
            {
                return (listno == 0) ? 120 : 0;
            }
        };
    }
}
