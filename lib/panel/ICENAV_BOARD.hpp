/**
 * @file ICENAV_BOARD.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for IceNaV board
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define LARGE_SCREEN
#define TOUCH_INPUT
#define SPLASH_FULLSCREEN

#define PANEL_TYPE            lgfx::Panel_ILI9488
#define PANEL_BUS_PARALLEL16
#define PANEL_FREQ_WRITE      80000000
#define PANEL_PIN_WR          TFT_WR
#define PANEL_PIN_RD          TFT_RD
#define PANEL_PIN_RS          TFT_RS
#define PANEL_PIN_D0          TFT_D0
#define PANEL_PIN_D1          TFT_D1
#define PANEL_PIN_D2          TFT_D2
#define PANEL_PIN_D3          TFT_D3
#define PANEL_PIN_D4          TFT_D4
#define PANEL_PIN_D5          TFT_D5
#define PANEL_PIN_D6          TFT_D6
#define PANEL_PIN_D7          TFT_D7
#define PANEL_PIN_D8          TFT_D8
#define PANEL_PIN_D9          TFT_D9
#define PANEL_PIN_D10         TFT_D10
#define PANEL_PIN_D11         TFT_D11
#define PANEL_PIN_D12         TFT_D12
#define PANEL_PIN_D13         TFT_D13
#define PANEL_PIN_D14         TFT_D14
#define PANEL_PIN_D15         TFT_D15
#define PANEL_PIN_CS          -1
#define PANEL_PIN_RST         -1
#define PANEL_WIDTH           320
#define PANEL_HEIGHT          480
#define PANEL_INVERT          true

#define TOUCH_FT5x06
#define TOUCH_PIN_INT         TCH_I2C_INT
#define TOUCH_PIN_SDA         I2C_SDA_PIN
#define TOUCH_PIN_SCL         I2C_SCL_PIN
#define TOUCH_X_MAX           319
#define TOUCH_Y_MAX           479

#include "lgfxCommon.hpp"
