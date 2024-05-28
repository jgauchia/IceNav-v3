/**
 * @file tft.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief TFT definition and functions
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef TFT_HPP
#define TFT_HPP

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>

#ifdef CUSTOMBOARD
#include <lgfx_CUSTOMBOARD_conf.hpp>
#endif

#ifdef MAKERF_ESP32S3
#include <lgfx_MAKERF_ESP32S3_conf.hpp>
//#include <lgfx_ESP32S3_TEST_conf.hpp>
#endif

#include <LGFX_TFT_eSPI.hpp>

extern TFT_eSPI tft;
static const char* calibrationFile PROGMEM = "/TouchCalData1";
extern bool repeatCalib;
static uint8_t brightnessLevel = 255;

void setBrightness(uint8_t brightness);
uint8_t getBrightness();
void tftOn();
void tftOff();
void touchCalibrate();
void initTFT();

#endif
