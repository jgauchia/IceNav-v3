/**
 * @file firmUpgrade.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Firmware upgrade from SD functions
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef FIRMUPGRADE_HPP
#define FIRMUPGRADE_HPP

#include <Update.h>
#include "tft.hpp"
#include "lvgl.h"
#include "upgradeScr.hpp"
#include "SD.h"

static const char *upgrdFile PROGMEM = "/firmware.bin"; // Firmware upgrade file

bool checkFileUpgrade();
void onUpgrdStart();
void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percent, uint16_t frameColor, uint16_t barColor);
void onUpgrdProcess(size_t currSize, size_t totalSize);
void onUpgrdEnd();

#endif