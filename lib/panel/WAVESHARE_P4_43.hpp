/**
 * @file WAVESHARE_P4_43.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for Waveshare ESP32-P4-Touch-LCD-4.3 (ST7701 MIPI-DSI + GT911)
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#define LARGE_SCREEN
#define EXTRA_LARGE_SCREEN
#define TOUCH_INPUT
#define SPLASH_FULLSCREEN

#define PANEL_TYPE            lgfx::Panel_ST7701_DSI
#define PANEL_BUS_DSI
#define PANEL_PIN_RST          TFT_RST
#define PANEL_PIN_BL           TFT_BL
#define PANEL_LIGHT_FREQ       5000
#define PANEL_LIGHT_INVERT     true
#define PANEL_WIDTH            480
#define PANEL_HEIGHT           800
#define PANEL_INVERT           false
#define PANEL_RGB_ORDER        true

#define PANEL_DSI_LANE_NUM     2
#define PANEL_DSI_LANE_MBPS    500
#define PANEL_DSI_LDO_CHAN     3
#define PANEL_DSI_LDO_MV       2500

#define PANEL_DSI_DPI_FREQ_MHZ        30
#define PANEL_DSI_HSYNC_BACK_PORCH    42
#define PANEL_DSI_HSYNC_PULSE_WIDTH   12
#define PANEL_DSI_HSYNC_FRONT_PORCH   42
#define PANEL_DSI_VSYNC_BACK_PORCH    2
#define PANEL_DSI_VSYNC_PULSE_WIDTH   8
#define PANEL_DSI_VSYNC_FRONT_PORCH   60

#define TOUCH_GT911
#define TOUCH_PIN_INT          -1
#define TOUCH_PIN_RST          TCH_I2C_RST
#define TOUCH_PIN_SDA          I2C_SDA_PIN
#define TOUCH_PIN_SCL          I2C_SCL_PIN
#define I2C_PORT               1
#define TOUCH_I2C_ADDR         0x14
#define TOUCH_X_MAX            479
#define TOUCH_Y_MAX            799

#include "lgfxCommon.hpp"
