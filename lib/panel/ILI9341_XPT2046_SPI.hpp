/**
 * @file ILI9341_XPT2046_SPI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9341 SPI With XPT2046 Touch controller
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define TOUCH_INPUT

#define PANEL_TYPE            lgfx::Panel_ILI9341
#define PANEL_BUS_SPI
#define PANEL_FREQ_WRITE      79999999
#define PANEL_FREQ_READ       27000000
#define PANEL_WIDTH           240
#define PANEL_HEIGHT          320

#define TOUCH_XPT2046
#define TOUCH_SPI_HOST        SPI3_HOST
#define TOUCH_X_MAX           240
#define TOUCH_Y_MAX           320

#include "lgfxCommon.hpp"
