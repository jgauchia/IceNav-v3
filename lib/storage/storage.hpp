/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8
 * @date 2024-11
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include <SD.h>
#include <LovyanGFX.hpp>
#include <tft.hpp>

static uint32_t sdFreq = 40000000;

extern bool isSdLoaded;

void initSD();
esp_err_t initSPIFFS();

#endif
