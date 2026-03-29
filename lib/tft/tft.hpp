/**
 * @file tft.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.2.5
 * @date 2026-04
 */

#pragma once

#include <stdint.h>
#include "storage.hpp"
#include "panelSelect.hpp"

#ifdef T4_S3
#include "LILYGO_T4_S3.hpp"
#endif


#include <LGFX_TFT_eSPI.hpp>

extern TFT_eSPI tft;                                      /**< TFT display object */
static const char* calibrationFile = "/spiffs/TouchCal"; /**< Touch calibration file path */
extern bool repeatCalib;                                 /**< Flag to repeat touch calibration */

extern uint16_t TFT_WIDTH;                               /**< TFT display width in pixels */
extern uint16_t TFT_HEIGHT;                              /**< TFT display height in pixels */
extern bool waitScreenRefresh;                           /**< Wait for refresh screen  */

void tftOn(uint8_t brightness);
void tftOff();
void touchCalibrate();
void initTFT();
