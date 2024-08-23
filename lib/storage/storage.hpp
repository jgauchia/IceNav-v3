/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include <SD.h>
#include <LovyanGFX.hpp>
#include <tft.hpp>

#ifdef ARDUINO_ESP32S3_DEV
static SPIClass spiSD = SPIClass(HSPI);
static uint32_t sdFreq = 10000000;
#endif
#ifdef ARDUINO_ESP32_DEV
static SPIClass spiSD = SPIClass(VSPI);
static uint32_t sdFreq = 40000000;
#endif

extern bool isSdLoaded;


void initSD();
esp_err_t initSPIFFS();

#endif
