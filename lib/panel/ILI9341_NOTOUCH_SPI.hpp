/**
 * @file ILI9341_NOTOUCH_SPI.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9341 SPI With no touch
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define PANEL_TYPE            lgfx::Panel_ILI9341
#define PANEL_BUS_SPI
#define PANEL_FREQ_WRITE      79999999
#define PANEL_FREQ_READ       27000000
#define PANEL_WIDTH           240
#define PANEL_HEIGHT          320

#include "lgfxCommon.hpp"
