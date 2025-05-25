/**
 * @file ELECROW_ESP32.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ELECROW ESP32 3.5 Terminal
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#define LGFX_USE_V1

#define LARGE_SCREEN
#define TOUCH_INPUT

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488   _panel_instance;
  lgfx::Bus_Parallel16  _bus_instance;
  lgfx::Light_PWM       _light_instance;
  lgfx::Touch_FT5x06    _touch_instance;

public:
  LGFX(void)
  {
    {                                      
      auto cfg = _bus_instance.config(); 

      cfg.port = 0;              
      cfg.freq_write = 80000000; 
      cfg.pin_wr = GPIO_NUM_18;
      cfg.pin_rd = GPIO_NUM_48;
      cfg.pin_rs = GPIO_NUM_45;

      cfg.pin_d0 = GPIO_NUM_47;
      cfg.pin_d1 = GPIO_NUM_21;
      cfg.pin_d2 = GPIO_NUM_14;
      cfg.pin_d3 = GPIO_NUM_13;
      cfg.pin_d4 = GPIO_NUM_12;
      cfg.pin_d5 = GPIO_NUM_11;
      cfg.pin_d6 = GPIO_NUM_10;
      cfg.pin_d7 = GPIO_NUM_9;
      cfg.pin_d8 = GPIO_NUM_3;
      cfg.pin_d9 = GPIO_NUM_8;
      cfg.pin_d10 = GPIO_NUM_16;
      cfg.pin_d11 = GPIO_NUM_15;
      cfg.pin_d12 = GPIO_NUM_7;
      cfg.pin_d13 = GPIO_NUM_6;
      cfg.pin_d14 = GPIO_NUM_5;
      cfg.pin_d15 = GPIO_NUM_4;

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
      cfg.pin_bl = GPIO_NUM_46;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 319;
      cfg.y_min = 0;
      cfg.y_max = 479;
      cfg.pin_int = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda = GPIO_NUM_38;
      cfg.pin_scl = GPIO_NUM_39;
      cfg.freq = 400000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};
