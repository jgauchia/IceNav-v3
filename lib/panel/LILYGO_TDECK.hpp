/**
 * @file LILYGO_TDECK.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) and Antonio Vanegas @Hpsturn
 * @brief  LOVYANGFX TFT driver for Lilygo T-DECK biard
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#define LGFX_USE_V1

#define TOUCH_INPUT

#include "LovyanGFX.hpp"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_GT911   _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.use_lock = false;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = GPIO_NUM_40;
      cfg.pin_mosi = GPIO_NUM_41;
      cfg.pin_miso = GPIO_NUM_38;
      cfg.pin_dc = GPIO_NUM_11;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = GPIO_NUM_12;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 16;
      cfg.dummy_read_bits = 2;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = GPIO_NUM_42;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    
    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 239;
      cfg.y_min = 0;
      cfg.y_max = 319;
      cfg.pin_int = GPIO_NUM_16;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x5D;   
      cfg.pin_sda = GPIO_NUM_18;
      cfg.pin_scl = GPIO_NUM_8;
      cfg.freq = 400000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};