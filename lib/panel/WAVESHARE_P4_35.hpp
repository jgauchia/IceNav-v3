/**
 * @file WAVESHARE_P4_35.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for Waveshare ESP32-P4-Touch-LCD-3.5 (ST7796 SPI + FT6336)
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#define LARGE_SCREEN
#define TOUCH_INPUT

#define PANEL_TYPE            lgfx::Panel_ST7796
#define PANEL_BUS_SPI
#define PANEL_FREQ_WRITE      40000000   // BSP uses 80 MHz; start conservative, raise after validating
#define PANEL_FREQ_READ       16000000
#define PANEL_PIN_SCLK        TFT_SCLK
#define PANEL_PIN_MOSI        TFT_MOSI
#define PANEL_PIN_MISO        -1
#define PANEL_PIN_DC          TFT_DC
#define PANEL_PIN_CS          TFT_CS
#define PANEL_PIN_RST         TFT_RST
#define PANEL_PIN_BL          TFT_BL
#define PANEL_WIDTH           320
#define PANEL_HEIGHT          480
#define PANEL_INVERT          true       // BSP: esp_lcd_panel_invert_color(true)

#define TOUCH_FT5x06
#define TOUCH_PIN_INT         TCH_I2C_INT
#define TOUCH_PIN_SDA         I2C_SDA_PIN
#define TOUCH_PIN_SCL         I2C_SCL_PIN
#define TOUCH_X_MAX           319
#define TOUCH_Y_MAX           479

#include "lgfxCommon.hpp"
