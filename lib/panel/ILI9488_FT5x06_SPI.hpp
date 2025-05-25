/**
 * @file ILI9488_XPT2046_SPI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9488 SPI With XPT2046 Touch controller
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#define LGFX_USE_V1

#include "LovyanGFX.hpp"

extern const uint8_t TFT_SPI_SCLK;
extern const uint8_t TFT_SPI_MOSI;
extern const uint8_t TFT_SPI_MISO;
extern const uint8_t TFT_SPI_DC;
extern const uint8_t TFT_SPI_CS;
extern const uint8_t TFT_SPI_RST;
extern const uint8_t TCH_I2C_PORT;
extern const uint8_t TCH_I2C_SDA;
extern const uint8_t TCH_I2C_SCL;
extern const uint8_t TCH_I2C_INT;
extern const bool TFT_INVERT;

#define LARGE_SCREEN
#define TOUCH_INPUT

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      #ifdef ICENAV_BOARD
      cfg.spi_host = SPI2_HOST;
      #endif
      #ifdef ESP32S3_N16R8
      cfg.spi_host = SPI2_HOST;
      #endif
      #ifdef ESP32_N16R4
      cfg.spi_host = HSPI_HOST;
      #endif
      cfg.spi_mode = 0;
      cfg.freq_write = 79999999;
      cfg.freq_read = 15000000;
      cfg.spi_3wire = false;
      cfg.use_lock = false;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SPI_SCLK;
      cfg.pin_mosi = TFT_SPI_MOSI;
      cfg.pin_miso = -1;
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
      #ifdef SPI_SHARED
      cfg.bus_shared = true;
      #endif
      #ifndef SPI_SHARED
      cfg.bus_shared = false;
      #endif
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = TFT_BL;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 320;
      cfg.y_min = 0;
      cfg.y_max = 480;
      cfg.pin_int = TCH_I2C_INT;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = TCH_I2C_PORT;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda = TCH_I2C_SDA;
      cfg.pin_scl = TCH_I2C_SCL;
      cfg.freq = 400000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};
