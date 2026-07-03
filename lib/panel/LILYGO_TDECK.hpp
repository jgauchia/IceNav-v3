/**
 * @file LILYGO_TDECK.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) and Antonio Vanegas @Hpsturn
 * @brief  LOVYANGFX TFT driver for Lilygo T-DECK board
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define TOUCH_INPUT

#define PANEL_TYPE             lgfx::Panel_ST7789
#define PANEL_BUS_SPI
#define PANEL_FREQ_WRITE       40000000
#define PANEL_FREQ_READ        16000000
#define PANEL_PIN_RST          -1
#define PANEL_WIDTH            240
#define PANEL_HEIGHT           320
#define PANEL_DUMMY_READ_PIXEL 16
#define PANEL_DUMMY_READ_BITS  2

#define TOUCH_GT911
#define TOUCH_I2C_ADDR         0x5D
#define TOUCH_PIN_INT          TCH_I2C_INT
#define TOUCH_PIN_SDA          I2C_SDA_PIN
#define TOUCH_PIN_SCL          I2C_SCL_PIN
#define TOUCH_X_MAX            239
#define TOUCH_Y_MAX            319

#include "lgfxCommon.hpp"
