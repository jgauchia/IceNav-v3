/**
 * @file ILI9488_XPT2046_SPI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9488 SPI With XPT2046 Touch controller
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
#define PANEL_WIDTH           320
#define PANEL_HEIGHT          480
#define PANEL_INVERT          TFT_INVERT

#define TOUCH_XPT2046
#define TOUCH_SPI_HOST        SPI2_HOST
#define TOUCH_X_MAX           320
#define TOUCH_Y_MAX           480

#include "lgfxCommon.hpp"
