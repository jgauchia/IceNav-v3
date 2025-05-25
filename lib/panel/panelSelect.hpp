/**
 * @file panelSelect.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Panel model select
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#if defined(ILI9488_XPT2046_SPI)
    #include "ILI9488_XPT2046_SPI.hpp"
#elif defined(ILI9488_FT5x06_16B)
    #include "ILI9488_FT5x06_16B.hpp"
#elif defined(ILI9341_XPT2046_SPI)
    #include "ILI9341_XPT2046_SPI.hpp"
#elif defined(ILI9488_FT5x06_SPI)
    #include "ILI9488_FT5x06_SPI.hpp"
#elif defined(ILI9488_NOTOUCH_8B)
    #include "ILI9488_NOTOUCH_8B.hpp"
#elif defined(ICENAV_BOARD)
    #include "ICENAV_BOARD.hpp"
#elif defined(TDECK_ESP32S3)
    #include "LILYGO_TDECK.hpp"
#else
    #error "No Panel defined!"
#endif