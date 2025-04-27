/**
 * @file LILYGO_TDECK.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) and Antonio Vanegas @Hpsturn
 * @brief  LOVYANGFX TFT driver for Lilygo T-DECK biard
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#ifndef LILYGO_TDECK_HPP
#define LILYGO_TDECK_HPP

#define LGFX_USE_V1

#define TOUCH_INPUT

#include "LovyanGFX.hpp"

// extern const uint8_t TFT_SPI_SCLK;
// extern const uint8_t TFT_SPI_MOSI;
// extern const uint8_t TFT_SPI_MISO;
// extern const uint8_t TFT_SPI_DC;
// extern const uint8_t TFT_SPI_CS;
// extern const uint8_t TFT_SPI_RST;
// extern const uint8_t TFT_SPI_BL;
// extern const uint8_t TCH_I2C_PORT;
// extern const uint8_t TCH_I2C_SDA;
// extern const uint8_t TCH_I2C_SCL;
// extern const uint8_t TCH_I2C_INT;
// extern const bool TFT_INVERT;

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_GT911   _touch_instance;

public:
  LGFX(void)
  {
    // Power on display 
    lgfx::pinMode(GPIO_NUM_10, lgfx::pin_mode_t::output);
    lgfx::gpio_hi(GPIO_NUM_10 );

    // Wakeup touch chip 
    lgfx::pinMode(GPIO_NUM_16, lgfx::pin_mode_t::output);
    lgfx::gpio_hi(GPIO_NUM_16 );
    delay(20);

    // Set touch int input 
    lgfx::pinMode(GPIO_NUM_16, lgfx::pin_mode_t::input);
    delay(20);

    // SD card 
    lgfx::pinMode(GPIO_NUM_39, lgfx::pin_mode_t::output);
    lgfx::gpio_hi(GPIO_NUM_39 );
    lgfx::pinMode(GPIO_NUM_38, lgfx::pin_mode_t::input_pullup);

    // Wakeup LoRa 
    lgfx::pinMode(GPIO_NUM_9,  lgfx::pin_mode_t::output);
    lgfx::gpio_hi(GPIO_NUM_9 );

    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 3;
      cfg.use_lock = true;
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

#endif
