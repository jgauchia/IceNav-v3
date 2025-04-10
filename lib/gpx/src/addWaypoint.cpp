/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0
 * @date 2025-04
 */

#include "addWaypoint.hpp"
#include "esp_log.h"

wayPoint addWpt;
wayPoint loadWpt; 
extern Storage storage;

static const char* TAG PROGMEM = "GPX Creation file";

/**
 * @brief Open GPX File exists if not, create new.
 * 
 * @param gpxFilename -> GPX file
 */
void createGpxFile(const char *gpxFilename)
{
  if (!storage.exists(wptFolder))
  {
    ESP_LOGI(TAG,"WPT folder not exists");
    if (storage.mkdir(wptFolder))
      ESP_LOGI(TAG, "WPT folder created");
    else
      ESP_LOGE(TAG, "WPT folder not created");
  }

  FILE *gpxFile = storage.open(gpxFilename, "r");

  if (!gpxFile)
  {
    ESP_LOGI(TAG,"GPX File not exists");
    gpxFile = storage.open(gpxFilename, "w");
    if (gpxFile)
    {
      ESP_LOGI(TAG,"Creating GPX File");
      storage.println(gpxFile, gpxType.header);
      storage.print(gpxFile, gpxType.footer);
      storage.close(gpxFile);
      ESP_LOGI(TAG,"File Size: %d", storage.size(gpxFilename));
    }
    else
      ESP_LOGE(TAG,"GPX File creation error");
  }
  else
  {
    ESP_LOGI(TAG,"GPX File exists");
    storage.close(gpxFile);
  }
}