/**
 * @file lgfxCommon.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Common LOVYANGFX device class built from per-board configuration macros
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "../../include/hal.hpp"

#ifndef PANEL_OFFSET_X
    #define PANEL_OFFSET_X 0
#endif
#ifndef PANEL_OFFSET_Y
    #define PANEL_OFFSET_Y 0
#endif
#ifndef PANEL_OFFSET_ROTATION
    #define PANEL_OFFSET_ROTATION 0
#endif
#ifndef PANEL_DUMMY_READ_PIXEL
    #define PANEL_DUMMY_READ_PIXEL 8
#endif
#ifndef PANEL_DUMMY_READ_BITS
    #define PANEL_DUMMY_READ_BITS 1
#endif
#ifndef PANEL_INVERT
    #define PANEL_INVERT false
#endif
#ifndef PANEL_RGB_ORDER
    #define PANEL_RGB_ORDER false
#endif
#ifndef PANEL_PIN_BL
    #define PANEL_PIN_BL TFT_BL
#endif
#ifndef PANEL_BUS_SHARED
    #if defined(PANEL_BUS_SPI) && !defined(SPI_SHARED)
        #define PANEL_BUS_SHARED false
    #else
        #define PANEL_BUS_SHARED true
    #endif
#endif

#if defined(PANEL_BUS_SPI)
    #ifndef PANEL_PIN_SCLK
        #define PANEL_PIN_SCLK TFT_SPI_SCLK
    #endif
    #ifndef PANEL_PIN_MOSI
        #define PANEL_PIN_MOSI TFT_SPI_MOSI
    #endif
    #ifndef PANEL_PIN_MISO
        #define PANEL_PIN_MISO TFT_SPI_MISO
    #endif
    #ifndef PANEL_PIN_DC
        #define PANEL_PIN_DC TFT_SPI_DC
    #endif
    #ifndef PANEL_PIN_CS
        #define PANEL_PIN_CS TFT_SPI_CS
    #endif
    #ifndef PANEL_PIN_RST
        #define PANEL_PIN_RST TFT_SPI_RST
    #endif
#endif

#if defined(TOUCH_XPT2046)
    #ifndef TOUCH_FREQ
        #define TOUCH_FREQ 1000000
    #endif
#elif defined(TOUCH_FT5x06) || defined(TOUCH_GT911)
    #ifndef TOUCH_FREQ
        #define TOUCH_FREQ 400000
    #endif
    #ifndef TOUCH_I2C_ADDR
        #define TOUCH_I2C_ADDR 0x38
    #endif
    #ifndef TOUCH_I2C_PORT
        #define TOUCH_I2C_PORT 0
    #endif
#endif
#ifndef TOUCH_OFFSET_ROTATION
    #define TOUCH_OFFSET_ROTATION 0
#endif

#if defined(TOUCH_XPT2046)
    #define TOUCH_RESISTIVE
#elif defined(TOUCH_FT5x06) || defined(TOUCH_GT911)
    #define TOUCH_CAPACITIVE
#endif

class LGFX : public lgfx::LGFX_Device
{
    PANEL_TYPE panelInstance;

#if defined(PANEL_BUS_SPI)
    lgfx::Bus_SPI busInstance;
#elif defined(PANEL_BUS_PARALLEL8)
    lgfx::Bus_Parallel8 busInstance;
#elif defined(PANEL_BUS_PARALLEL16)
    lgfx::Bus_Parallel16 busInstance;
#else
    #error "No panel bus defined!"
#endif

    lgfx::Light_PWM lightInstance;

#if defined(TOUCH_XPT2046)
    lgfx::Touch_XPT2046 touchInstance;
#elif defined(TOUCH_FT5x06)
    lgfx::Touch_FT5x06 touchInstance;
#elif defined(TOUCH_GT911)
    lgfx::Touch_GT911 touchInstance;
#endif

    public:
        LGFX(void)
        {
#if defined(PANEL_BUS_SPI)
            {
                auto cfg = busInstance.config();
                cfg.spi_host = SPI2_HOST;
                cfg.spi_mode = 0;
                cfg.freq_write = PANEL_FREQ_WRITE;
                cfg.freq_read = PANEL_FREQ_READ;
                cfg.spi_3wire = false;
                cfg.use_lock = false;
                cfg.dma_channel = SPI_DMA_CH_AUTO;
                cfg.pin_sclk = PANEL_PIN_SCLK;
                cfg.pin_mosi = PANEL_PIN_MOSI;
                cfg.pin_miso = PANEL_PIN_MISO;
                cfg.pin_dc = PANEL_PIN_DC;
                busInstance.config(cfg);
                panelInstance.setBus(&busInstance);
            }
#else
            {
                auto cfg = busInstance.config();
                cfg.port = 0;
                cfg.freq_write = PANEL_FREQ_WRITE;
                cfg.pin_wr = PANEL_PIN_WR;
                cfg.pin_rd = PANEL_PIN_RD;
                cfg.pin_rs = PANEL_PIN_RS;
                cfg.pin_d0 = PANEL_PIN_D0;
                cfg.pin_d1 = PANEL_PIN_D1;
                cfg.pin_d2 = PANEL_PIN_D2;
                cfg.pin_d3 = PANEL_PIN_D3;
                cfg.pin_d4 = PANEL_PIN_D4;
                cfg.pin_d5 = PANEL_PIN_D5;
                cfg.pin_d6 = PANEL_PIN_D6;
                cfg.pin_d7 = PANEL_PIN_D7;
                #if defined(PANEL_BUS_PARALLEL16)
                    cfg.pin_d8 = PANEL_PIN_D8;
                    cfg.pin_d9 = PANEL_PIN_D9;
                    cfg.pin_d10 = PANEL_PIN_D10;
                    cfg.pin_d11 = PANEL_PIN_D11;
                    cfg.pin_d12 = PANEL_PIN_D12;
                    cfg.pin_d13 = PANEL_PIN_D13;
                    cfg.pin_d14 = PANEL_PIN_D14;
                    cfg.pin_d15 = PANEL_PIN_D15;
                #endif
                busInstance.config(cfg);
                panelInstance.setBus(&busInstance);
            }
#endif

            {
                auto cfg = panelInstance.config();
                cfg.pin_cs = PANEL_PIN_CS;
                cfg.pin_rst = PANEL_PIN_RST;
                cfg.pin_busy = -1;
                cfg.panel_width = PANEL_WIDTH;
                cfg.panel_height = PANEL_HEIGHT;
                cfg.memory_width = PANEL_WIDTH;
                cfg.memory_height = PANEL_HEIGHT;
                cfg.offset_x = PANEL_OFFSET_X;
                cfg.offset_y = PANEL_OFFSET_Y;
                cfg.offset_rotation = PANEL_OFFSET_ROTATION;
                cfg.dummy_read_pixel = PANEL_DUMMY_READ_PIXEL;
                cfg.dummy_read_bits = PANEL_DUMMY_READ_BITS;
                cfg.readable = true;
                cfg.invert = PANEL_INVERT;
                cfg.rgb_order = PANEL_RGB_ORDER;
                #if defined(PANEL_BUS_PARALLEL16)
                    cfg.dlen_16bit = true;
                #else
                    cfg.dlen_16bit = false;
                #endif
                cfg.bus_shared = PANEL_BUS_SHARED;
                panelInstance.config(cfg);
            }

            {
                auto cfg = lightInstance.config();
                cfg.pin_bl = PANEL_PIN_BL;
                cfg.invert = false;
                cfg.freq = 44100;
                cfg.pwm_channel = 7;
                lightInstance.config(cfg);
                panelInstance.setLight(&lightInstance);
            }

#if defined(TOUCH_XPT2046)
            {
                auto cfg = touchInstance.config();
                cfg.x_min = 0;
                cfg.x_max = TOUCH_X_MAX;
                cfg.y_min = 0;
                cfg.y_max = TOUCH_Y_MAX;
                cfg.pin_int = TCH_SPI_INT;
                cfg.bus_shared = true;
                cfg.offset_rotation = TOUCH_OFFSET_ROTATION;
                cfg.spi_host = TOUCH_SPI_HOST;
                cfg.freq = TOUCH_FREQ;
                cfg.pin_sclk = TCH_SPI_SCLK;
                cfg.pin_mosi = TCH_SPI_MOSI;
                cfg.pin_miso = TCH_SPI_MISO;
                cfg.pin_cs = TCH_SPI_CS;
                touchInstance.config(cfg);
                panelInstance.setTouch(&touchInstance);
            }
#elif defined(TOUCH_FT5x06) || defined(TOUCH_GT911)
            {
                auto cfg = touchInstance.config();
                cfg.x_min = 0;
                cfg.x_max = TOUCH_X_MAX;
                cfg.y_min = 0;
                cfg.y_max = TOUCH_Y_MAX;
                cfg.pin_int = TOUCH_PIN_INT;
                cfg.bus_shared = true;
                cfg.offset_rotation = TOUCH_OFFSET_ROTATION;
                cfg.i2c_port = TOUCH_I2C_PORT;
                cfg.i2c_addr = TOUCH_I2C_ADDR;
                cfg.pin_sda = TOUCH_PIN_SDA;
                cfg.pin_scl = TOUCH_PIN_SCL;
                cfg.freq = TOUCH_FREQ;
                #ifdef TOUCH_PIN_RST
                    cfg.pin_rst = TOUCH_PIN_RST;
                #endif
                touchInstance.config(cfg);
                panelInstance.setTouch(&touchInstance);
            }
#endif
            setPanel(&panelInstance);
        }
};
