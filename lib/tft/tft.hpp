/**
 * @file tft.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief TFT definition and functions
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef TFT_HPP
#define TFT_HPP

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>

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

#include <LGFX_TFT_eSPI.hpp>

extern TFT_eSPI tft;
static const char* calibrationFile PROGMEM = "/TouchCalData1";
extern bool repeatCalib;
static uint8_t brightnessLevel = 255;
extern uint16_t TFT_WIDTH;
extern uint16_t TFT_HEIGHT;
extern bool waitScreenRefresh;                  // Wait for refres screen (screenshoot issues)


void setBrightness(uint8_t brightness);
uint8_t getBrightness();
void tftOn();
void tftOff();
void touchCalibrate();
void initTFT();

#endif
