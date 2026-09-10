/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include "globalGuiDef.h"
#include "settings.hpp"
#include "maps.hpp"
#include "gps.hpp"

#ifdef WAVESHARE_P4_43
    inline constexpr const char* logoFile = "/spiffs/gfx/LOGO_460_800.png"; /**< Path to the large logo image for large screens */
#elif defined(LARGE_SCREEN)
    #ifdef SPLASH_FULLSCREEN
        inline constexpr const char* logoFile = "/spiffs/gfx/LOGO_NEW.png"; /**< Path to the large logo image for large screens */
    #else
        inline constexpr const char* logoFile = "/spiffs/gfx/LOGO_LARGE.png"; /**< Path to the large logo image for large screens */
    #endif
#else
    inline constexpr const char* logoFile = "/spiffs/gfx/LOGO_SMALL.png"; /**< Path to the small logo image for smaller screens */
#endif

inline constexpr const char* statusLine1 = "Model:%s %dMhz";             /**< Format string for model and CPU frequency */
inline constexpr const char* statusLine2 = "Free mem:%dK %d%%";          /**< Format string for free memory in KB and percentage */
inline constexpr const char* statusLine3 = "PSRAM: %d - Used PSRAM: %d"; /**< Format string for PSRAM total and used */
inline constexpr const char* statusLine4 = "Firmware v.%s rev.%s";       /**< Format string for firmware version and revision */
inline constexpr const char* statusLine5 = "ENV: %s";                    /**< Format string for environment information */

extern lv_obj_t *splashScr;
extern lv_obj_t *splashCanvas;

void createLVGLSplashScreen();
void splashScreen();
