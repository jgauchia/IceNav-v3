/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include <SD.h>
#include <LovyanGFX.hpp>
#include <tft.hpp>

#ifdef MAKERF_ESP32S3
static SPIClass spiSD = SPIClass(HSPI);
static uint32_t sdFreq = 10000000;
#endif
#ifdef ELECROW_ESP32
static SPIClass spiSD = SPIClass(HSPI);
static uint32_t sdFreq = 10000000;
#endif
#ifdef ESP32S3_N16R8
static SPIClass spiSD = SPIClass(HSPI);
static uint32_t sdFreq = 10000000;
#endif
#ifdef ESP32_N16R4
static SPIClass spiSD = SPIClass(VSPI);
static uint32_t sdFreq = 40000000;
#endif

extern bool isSdLoaded;

void initSD();
esp_err_t initSPIFFS();
void acquireSdSPI();
void releaseSdSPI();

#endif
