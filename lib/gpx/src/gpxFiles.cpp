/**
 * @file gpxFiles.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Create GPX files and folder struct
 * @version 0.2.1
 * @date 2025-05
 */

#include "gpxFiles.hpp"
#include "esp_log.h"

wayPoint addWpt;
wayPoint loadWpt; 
extern Storage storage;

static const char* TAG PROGMEM = "GPX file struct";

/**
 * @brief Create GPX folders structure
 * 
 */
void createGpxFolders()
{
  if (!storage.exists(trkFolder))
  {
    ESP_LOGI(TAG,"TRK folder not exists");
    if (storage.mkdir(trkFolder))
      ESP_LOGI(TAG, "TRK folder created");
    else
      ESP_LOGE(TAG, "TRK folder not created");
  }
  else
    ESP_LOGI(TAG,"TRK folder exists");

  if (!storage.exists(wptFolder))
  {
    ESP_LOGI(TAG,"WPT folder not exists");
    if (storage.mkdir(wptFolder))
      ESP_LOGI(TAG, "WPT folder created");
    else
      ESP_LOGE(TAG, "WPT folder not created");
  }
  else
    ESP_LOGI(TAG,"WPT folder exists");
}

/**
 * @brief Create default IcenNav waypoint file
 * 
 */
void createWptFile()
{
  FILE *gpxFile = storage.open(wptFile, "r");

  if (!gpxFile)
  {
    ESP_LOGI(TAG,"WPT file not exists");
    gpxFile = storage.open(wptFile, "w");
    if (gpxFile)
    {
      ESP_LOGI(TAG,"Creating WPT file");
      storage.println(gpxFile, gpxHeader);
      storage.close(gpxFile);
      ESP_LOGI(TAG,"file Size: %d", storage.size(wptFile));
    }
    else
      ESP_LOGE(TAG,"WPT file creation error");
  }
  else
  {
    ESP_LOGI(TAG,"WPT file exists");
    storage.close(gpxFile);
  }
}