/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include "globalGuiDef.h"
#include "settings.hpp"
#include "maps.hpp"
#include "gps.hpp"

#ifdef LARGE_SCREEN
    #ifdef ICENAV_BOARD
        static const char* logoFile PROGMEM = "/spiffs/LOGO_NEW.png"; /**< Path to the large logo image for large screens */  
    #else
        static const char* logoFile PROGMEM = "/spiffs/LOGO_LARGE.png"; /**< Path to the large logo image for large screens */
    #endif
#else
    static const char* logoFile PROGMEM = "/spiffs/LOGO_SMALL.png"; /**< Path to the small logo image for smaller screens */
#endif

static const char* statusLine1 PROGMEM = "Model:%s %dMhz";             /**< Format string for model and CPU frequency */
static const char* statusLine2 PROGMEM = "Free mem:%dK %d%%";          /**< Format string for free memory in KB and percentage */
static const char* statusLine3 PROGMEM = "PSRAM: %d - Used PSRAM: %d"; /**< Format string for PSRAM total and used */
static const char* statusLine4 PROGMEM = "Firmware v.%s rev.%s";       /**< Format string for firmware version and revision */
static const char* statusLine5 PROGMEM = "ENV: %s";                    /**< Format string for environment information */

extern lv_obj_t *splashScr;
static lv_obj_t *splashCanvas;

void createLVGLSplashScreen();
void splashScreen();
