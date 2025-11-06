/**
 * @file tft.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#include <Arduino.h>
#include "storage.hpp"
#include "panelSelect.hpp"

#include <LGFX_TFT_eSPI.hpp>

extern TFT_eSPI tft;                                      /**< TFT display object */
static const char* calibrationFile PROGMEM = "/spiffs/TouchCal"; /**< Touch calibration file path */
extern bool repeatCalib;                                 /**< Flag to repeat touch calibration */

extern uint16_t TFT_WIDTH;                               /**< TFT display width in pixels */
extern uint16_t TFT_HEIGHT;                              /**< TFT display height in pixels */
extern bool waitScreenRefresh;                           /**< Wait for refresh screen  */

void tftOn(uint8_t brightness);
void tftOff();
void touchCalibrate();
void initTFT();
