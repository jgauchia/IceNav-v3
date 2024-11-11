/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-11
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include <SD.h>
#include <LovyanGFX.hpp>
#include <tft.hpp>

#if defined ( ICENAV_BOARD ) || defined ( MAKERF_ESP32S3 ) || defined ( ELECROW_ESP32 )
static SPIClass spiSD = SPIClass(HSPI);
static uint32_t sdFreq = 10000000;
#endif

extern bool isSdLoaded;

void initSD();
esp_err_t initSPIFFS();

#endif
