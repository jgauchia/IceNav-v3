/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.2.0
 * @date 2024-12
 */

#ifndef SPLASHSCR_HPP
#define SPLASHSCR_HPP

#include "globalGuiDef.h"
#include "settings.hpp"

#ifdef LARGE_SCREEN
static const char* logoFile PROGMEM = "/spiffs/LOGO_LARGE.png";
#else
static const char* logoFile PROGMEM = "/spiffs/LOGO_SMALL.png";
#endif
static const char* statusLine1 PROGMEM = "Model:%s %dMhz";
static const char* statusLine2 PROGMEM = "Free mem:%dK %d%%";
static const char* statusLine3 PROGMEM = "PSRAM: %d - Used PSRAM: %d";
static const char* statusLine4 PROGMEM = "Firmware v.%s rev.%s";
static const char* statusLine5 PROGMEM = "ENV: %s";

void splashScreen();

#endif
