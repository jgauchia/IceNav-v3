/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.9
 * @date 2024-12
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
