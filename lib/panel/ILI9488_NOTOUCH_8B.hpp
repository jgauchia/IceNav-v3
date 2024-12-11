/**
 * @file ILI9488_NOTOUCH_8B.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LOVYANGFX TFT driver for ILI9488 8 Bits parallel  Without Touch controller
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef ILI9488_NOTOUCH_8B_HPP
#define ILI9488_NOTOUCH_8B_HPP

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

extern const uint8_t TFT_WR;
extern const uint8_t TFT_RD;
extern const uint8_t TFT_RS;
extern const uint8_t TFT_RST;
extern const uint8_t TFT_CS;

extern const uint8_t TFT_D0;
extern const uint8_t TFT_D1;
extern const uint8_t TFT_D2;
extern const uint8_t TFT_D3;
extern const uint8_t TFT_D4;
extern const uint8_t TFT_D5;
extern const uint8_t TFT_D6;
extern const uint8_t TFT_D7;

#define LARGE_SCREEN

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Bus_Parallel8 _bus_instance;

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

      _bus_instance.config(cfg);              
      _panel_instance.setBus(&_bus_instance); 
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
      auto cfg = _panel_instance.config();

      cfg.pin_cs = TFT_CS;   
      cfg.pin_rst = TFT_RST;  
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
      cfg.dlen_16bit = false;    
      cfg.bus_shared = true;    

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance); 

  }
};

#endif
