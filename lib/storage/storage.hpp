/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Storage definition and functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "esp_spiffs.h"
#include "esp_err.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class Storage
{
private:
    bool isSdLoaded;
    static const uint32_t sdFreq = 40000000;
    sdmmc_card_t *card;

public:
    Storage();

    esp_err_t initSD();
    esp_err_t initSPIFFS();
    bool getSdLoaded() const;
    FILE *open(const char *path, const char *mode);
    bool exists(const char *path);
    bool mkdir(const char *path);
    bool remove(const char *path);
    bool rmdir(const char *path);
};

#endif