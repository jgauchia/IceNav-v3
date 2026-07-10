/**
 * @file ILI9488_FT5x06_SPI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9488 SPI With FT5x06 Touch controller
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define LARGE_SCREEN
#define TOUCH_INPUT

#define PANEL_TYPE            lgfx::Panel_ILI9488
#define PANEL_BUS_SPI
#define PANEL_FREQ_WRITE      79999999
#define PANEL_FREQ_READ       15000000
#define PANEL_PIN_MISO        -1
#define PANEL_WIDTH           320
#define PANEL_HEIGHT          480
#define PANEL_INVERT          TFT_INVERT

#define TOUCH_FT5x06
#define I2C_PORT              TCH_I2C_PORT
#define TOUCH_PIN_INT         TCH_I2C_INT
#define TOUCH_PIN_SDA         TCH_I2C_SDA
#define TOUCH_PIN_SCL         TCH_I2C_SCL
#define TOUCH_X_MAX           320
#define TOUCH_Y_MAX           480

#include "lgfxCommon.hpp"
