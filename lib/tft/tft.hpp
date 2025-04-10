/**
 * @file tft.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef TFT_HPP
#define TFT_HPP

#include <Arduino.h>
#include "storage.hpp"

#ifdef ILI9488_XPT2046_SPI
#include "ILI9488_XPT2046_SPI.hpp"
#endif

#ifdef ILI9488_FT5x06_16B
#include "ILI9488_FT5x06_16B.hpp"
#endif

#ifdef ILI9341_XPT2046_SPI
#include "ILI9341_XPT2046_SPI.hpp"
#endif

#ifdef ILI9488_FT5x06_SPI
#include "ILI9488_FT5x06_SPI.hpp"
#endif

#ifdef ILI9488_NOTOUCH_8B
#include "ILI9488_NOTOUCH_8B.hpp"
#endif

#ifdef ICENAV_BOARD
#include "ICENAV_BOARD.hpp"
#endif

#ifdef TDECK_ESP32S3
#include "LILYGO_TDECK.hpp"
#endif

#include <LGFX_TFT_eSPI.hpp>

extern TFT_eSPI tft;
static const char* calibrationFile PROGMEM = "/spiffs/TouchCal";
extern bool repeatCalib;

extern uint16_t TFT_WIDTH;
extern uint16_t TFT_HEIGHT;
extern bool waitScreenRefresh;                  // Wait for refresh screen (screenshot issues)

void tftOn(uint8_t brightness);
void tftOff();
void touchCalibrate();
void initTFT();

#endif
