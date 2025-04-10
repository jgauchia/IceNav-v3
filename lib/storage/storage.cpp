/**
 * @file storage.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Storage definition and functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#include "storage.hpp"

// Pin definitions (should be configurable if needed)
extern const uint8_t SD_CS;
extern const uint8_t SD_MISO;
extern const uint8_t SD_MOSI;
extern const uint8_t SD_CLK;

static const char *TAG = "Storage";

/**
 * @brief Storage Class constructor
 *
 */
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
  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf =
      {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    else if (ret == ESP_ERR_NOT_FOUND)
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    else
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  else
    ESP_LOGI(TAG, "Partition size: total: %d used: %d", total, used);

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
