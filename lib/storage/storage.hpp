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
#include <SD.h>

class Storage
{
private:
    bool isSdLoaded;
    static const uint32_t sdFreq = 40000000;

public:
    Storage();

    void initSD();
    esp_err_t initSPIFFS();
    bool getSdLoaded() const;
};

#endif
