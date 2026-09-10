/**
 * @file ILI9488_NOTOUCH_8B.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LOVYANGFX TFT driver for ILI9488 8 Bits parallel  Without Touch controller
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#define LARGE_SCREEN

#define PANEL_TYPE            lgfx::Panel_ILI9488
#define PANEL_BUS_PARALLEL8
#define PANEL_FREQ_WRITE      20000000
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
#define PANEL_PIN_CS          TFT_CS
#define PANEL_PIN_RST         TFT_RST
#define PANEL_WIDTH           320
#define PANEL_HEIGHT          480

#include "lgfxCommon.hpp"
