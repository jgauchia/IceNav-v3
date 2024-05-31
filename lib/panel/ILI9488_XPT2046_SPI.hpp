/**
 * @file ILI9488_XPT2046_SPI.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LOVYANGFX TFT driver for ILI9488 SPI With XPT2046 Touch controller
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef ILI9488_XPT2046_SPI_HPP
#define ILI9488_XPT2046_SPI_HPP

#define LGFX_USE_V1

#include "LovyanGFX.hpp"

extern const uint8_t TFT_SPI_SCLK;
extern const uint8_t TFT_SPI_MOSI;
extern const uint8_t TFT_SPI_MISO;
extern const uint8_t TFT_SPI_DC;
extern const uint8_t TFT_SPI_CS;
extern const uint8_t TFT_SPI_RST;
extern const uint8_t TCH_SPI_SCLK;
extern const uint8_t TCH_SPI_MOSI;
extern const uint8_t TCH_SPI_MISO;
extern const uint8_t TCH_SPI_INT;
extern const uint8_t TCH_SPI_CS;
extern const bool TFT_INVERT;


class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Touch_XPT2046 _touch_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = HSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 79999999;
            //cfg.freq_write = 40000000;
            cfg.freq_read = 27000000;
            cfg.spi_3wire = true;
            cfg.use_lock = false;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = TFT_SPI_SCLK;
            cfg.pin_mosi = TFT_SPI_MOSI;
            cfg.pin_miso = TFT_SPI_MISO;
            cfg.pin_dc = TFT_SPI_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = TFT_SPI_CS;
            cfg.pin_rst = TFT_SPI_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = 320;
            cfg.panel_height = 480;
            cfg.memory_width = 320;
            cfg.memory_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = TFT_INVERT;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }

        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = 320;
            cfg.y_min = 0;
            cfg.y_max = 480;
            cfg.pin_int = 5;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;
            cfg.spi_host = HSPI_HOST;
            cfg.freq = 1000000;
            cfg.pin_sclk = TCH_SPI_SCLK;
            cfg.pin_mosi = TCH_SPI_MOSI;
            cfg.pin_miso = TCH_SPI_MISO;
            cfg.pin_cs = TCH_SPI_CS;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};

#endif
