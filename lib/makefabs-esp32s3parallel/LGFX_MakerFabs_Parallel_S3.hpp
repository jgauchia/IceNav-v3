/**
 * @file lovyangfx_CUSTOMBOARD_conf.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for MAKERF_ESP32S3
 * @version 0.1.4
 * @date 2023-05-23
 */

#define LGFX_USE_V1

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    static constexpr int I2C_PORT_NUM = 0;
    static constexpr int I2C_PIN_SDA = 38;
    static constexpr int I2C_PIN_SCL = 39;
    static constexpr int I2C_PIN_INT = 40;

    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance;
    lgfx::Touch_FT5x06 _touch_instance;

public:
    LGFX(void)
    {
        {                                      
            auto cfg = _bus_instance.config(); 

            cfg.port = 0;              
            cfg.freq_write = 20000000; 
            cfg.pin_wr = 35;           
            cfg.pin_rd = 48;           
            cfg.pin_rs = 36;          

            cfg.pin_d0 = 47;
            cfg.pin_d1 = 21;
            cfg.pin_d2 = 14;
            cfg.pin_d3 = 13;
            cfg.pin_d4 = 12;
            cfg.pin_d5 = 11;
            cfg.pin_d6 = 10;
            cfg.pin_d7 = 9;
            cfg.pin_d8 = 3;
            cfg.pin_d9 = 8;
            cfg.pin_d10 = 16;
            cfg.pin_d11 = 15;
            cfg.pin_d12 = 7;
            cfg.pin_d13 = 6;
            cfg.pin_d14 = 5;
            cfg.pin_d15 = 4;

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

        setPanel(&_panel_instance); 

        {
            auto cfg = _touch_instance.config();

            cfg.x_min = 0;
            cfg.x_max = 320;
            cfg.y_min = 0;
            cfg.y_max = 480;
            cfg.pin_int = I2C_PIN_INT;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;

            cfg.i2c_port = I2C_PORT_NUM;
            cfg.i2c_addr = 0x38;
            cfg.pin_sda = I2C_PIN_SDA;
            cfg.pin_scl = I2C_PIN_SCL;
            cfg.freq = 400000;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};
