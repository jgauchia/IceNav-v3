/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Splash screen - NOT LVGL
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef SPLASHSCR_HPP
#define SPLASHSCR_HPP

#include "tft.hpp"

static const char* logoFile PROGMEM = "/BOOTLOGO.png";
static const char* statusLine1 PROGMEM = "Model:%s %dMhz - Free mem:%dK %d%%";
static const char* statusLine2 PROGMEM = "PSRAM: %d - Used PSRAM: %d";
static const char* statusLine3 PROGMEM = "Firmware v.%s rev.%s - %s";

void splashScreen();

#endif