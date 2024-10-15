/**
 * @file storage.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "storage.hpp"
#include "esp_err.h"
#include "esp_spiffs.h"

bool isSdLoaded = false;
extern const int SD_CS;
extern const int SD_MISO;
extern const int SD_MOSI;
extern const int SD_CLK;
extern const int BOARD_TFT_CS;
extern const int RADIO_CS_PIN;

/**
 * @brief SD Card init
 *
 */
void initSD()
{
  bool SDInitOk = false;
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,LOW);

  #ifdef SPI_SHARED
    SD.end();
    SDInitOk = SD.begin(SD_CS);
  #endif

  #ifndef SPI_SHARED
    spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    SDInitOk = SD.begin(SD_CS, spiSD, sdFreq);
  #endif
  
  if (!SDInitOk)
  {
    log_e("SD Card Mount Failed");
    return;
  }
  else
  {
    log_v("SD Card Mounted");
    isSdLoaded = true;
  }
 }

/**
 * @brief SPIFFS Init
 *
 */
esp_err_t initSPIFFS()
{
  log_i("Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = false
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret !=ESP_OK)
  {
    if (ret == ESP_FAIL)
      log_e("Failed to mount or format filesystem");
    else if (ret == ESP_ERR_NOT_FOUND)
      log_e("Failed to find SPIFFS partition");
    else
      log_e("Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret!= ESP_OK)
    log_e("Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  else
    log_i("Partition size: total: %d used: %d", total, used);
  
  return ESP_OK;
}

/**
 * @brief Acquire SPI Bus for SD operations
 *
 */
 void acquireSdSPI()
 {
    #ifdef SPI_SHARED
    tft.waitDisplay();
    tft.endTransaction();
    digitalWrite(TFT_SPI_CS,HIGH);
    digitalWrite(SD_CS,LOW);
    #endif
 }

 /**
  * @brief Release SPI Bus for other operations
  *
  */
  void releaseSdSPI()
  {
    #ifdef SPI_SHARED   
    digitalWrite(SD_CS,HIGH);
    digitalWrite(TFT_SPI_CS,LOW);
    tft.beginTransaction();
    #endif  
  }
