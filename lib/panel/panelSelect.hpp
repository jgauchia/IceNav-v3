/**
 * @file panelSelect.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Panel model select
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

/**
 * @brief Include the appropriate display panel driver based on the defined macro
 *
 * @details This section conditionally includes the correct header for the selected display/touch panel hardware.
 *          If no supported panel macro is defined, compilation will stop with an error.
 */
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
#elif defined(ILI9341_NOTOUCH_SPI)
    #include "ILI9341_NOTOUCH_SPI.hpp"
#elif defined(ICENAV_BOARD)
    #include "ICENAV_BOARD.hpp"
#elif defined(TDECK_ESP32S3)
    #include "LILYGO_TDECK.hpp"
#elif defined(T4_S3)
    #include "LILYGO_T4_S3.hpp"
#elif defined(MAKERF_ESP32S3)
    #include "MAKERF_ESP32S3.hpp"
#elif defined(ELECROW_ESP32)
    #include "ELECROW_ESP32.hpp"
#else
    #error "No Panel defined!"
#endif