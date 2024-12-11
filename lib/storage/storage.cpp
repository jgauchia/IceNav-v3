/**
 * @file storage.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.9
 * @date 2024-12
 */

#include "storage.hpp"

// Pin definitions (should be configurable if needed)
extern const uint8_t SD_CS;
extern const uint8_t SD_MISO;
extern const uint8_t SD_MOSI;
extern const uint8_t SD_CLK;

Storage::Storage() : isSdLoaded(false) {}


/**
 * @brief SD Card init
 *
 */
void Storage::initSD() 
{
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, LOW);

    SPI.begin(SD_CLK, SD_MISO, SD_MOSI);
    
    if (!SD.begin(SD_CS, SPI, sdFreq)) 
    {
        log_e("SD Card Mount Failed");
        isSdLoaded = false;
    } 
    else 
    {
        log_v("SD Card Mounted");
        isSdLoaded = true;
    }
}

/**
 * @brief SPIFFS initialization
 *
 * @return esp_err_t Error code for SPIFFS setup
 */
esp_err_t Storage::initSPIFFS() 
{
    log_i("Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf =
    {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL)
            log_e("Failed to mount or format filesystem");
        else if (ret == ESP_ERR_NOT_FOUND)
            log_e("Failed to find SPIFFS partition");
        else
            log_e("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
        log_e("Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    else
        log_i("Partition size: total: %d used: %d", total, used);

    return ESP_OK;
}

/**
 * @brief Get SD status
 *
 * @return true if SD card is loaded, false otherwise
 */
bool Storage::getSdLoaded() const 
{
    return isSdLoaded;
}
