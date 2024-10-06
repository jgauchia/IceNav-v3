/**
 * @file ILI9488_FT5x06_16B.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LOVYANGFX TFT driver for ILI9488 16 Bits parallel  With FT5x06 Touch controller
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef ILI9488_FT5x06_16B_HPP
#define ILI9488_FT5x06_16B_HPP

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

extern const uint8_t TCH_I2C_PORT;
extern const uint8_t TCH_I2C_SDA;
extern const uint8_t TCH_I2C_SCL;
extern const uint8_t TCH_I2C_INT;
extern const uint8_t TFT_WR;
extern const uint8_t TFT_RD;
extern const uint8_t TFT_RS;
extern const uint8_t TFT_D0;
extern const uint8_t TFT_D1;
extern const uint8_t TFT_D2;
extern const uint8_t TFT_D3;
extern const uint8_t TFT_D4;
extern const uint8_t TFT_D5;
extern const uint8_t TFT_D6;
extern const uint8_t TFT_D7;
extern const uint8_t TFT_D8;
extern const uint8_t TFT_D9;
extern const uint8_t TFT_D10;
extern const uint8_t TFT_D11;
extern const uint8_t TFT_D12;
extern const uint8_t TFT_D13;
extern const uint8_t TFT_D14;
extern const uint8_t TFT_D15;

#define LARGE_SCREEN
#define TOUCH_INPUT

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_Parallel16 _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  LGFX(void)
  {
    {                                      
      auto cfg = _bus_instance.config(); 

      cfg.port = 0;              
      cfg.freq_write = 20000000; 
      cfg.pin_wr = TFT_WR;
      cfg.pin_rd = TFT_RD;
      cfg.pin_rs = TFT_RS;

      cfg.pin_d0 = TFT_D0;
      cfg.pin_d1 = TFT_D1;
      cfg.pin_d2 = TFT_D2;
      cfg.pin_d3 = TFT_D3;
      cfg.pin_d4 = TFT_D4;
      cfg.pin_d5 = TFT_D5;
      cfg.pin_d6 = TFT_D6;
      cfg.pin_d7 = TFT_D7;
      cfg.pin_d8 = TFT_D8;
      cfg.pin_d9 = TFT_D9;
      cfg.pin_d10 = TFT_D10;
      cfg.pin_d11 = TFT_D11;
      cfg.pin_d12 = TFT_D12;
      cfg.pin_d13 = TFT_D13;
      cfg.pin_d14 = TFT_D14;
      cfg.pin_d15 = TFT_D15;

      _bus_instance.config(cfg);              
      _panel_instance.setBus(&_bus_instance); 
    }

    {                                        
      auto cfg = _panel_instance.config();

      cfg.pin_cs = -1;   
      cfg.pin_rst = -1;  
      cfg.pin_busy = -1; 

      cfg.memory_width = 320;   
      cfg.memory_height = 480;  
      cfg.panel_width = 320;    
      cfg.panel_height = 480;   
      cfg.offset_x = 0;         
      cfg.offset_y = 0;         
      cfg.offset_rotation = 0;  
      cfg.dummy_read_pixel = 8; 
      cfg.dummy_read_bits = 1;  
      cfg.readable = true;      
      cfg.invert = false;       
      cfg.rgb_order = false;    
      cfg.dlen_16bit = true;    
      cfg.bus_shared = true;    

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

#endif
