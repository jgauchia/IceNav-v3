/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include <LovyanGFX.hpp>
#include <tft.hpp>

static sdmmc_card_t* sdcard;
extern bool isSdLoaded;



esp_err_t initSD();
esp_err_t initSPIFFS();
void acquireSdSPI();
void releaseSdSPI();

#endif
