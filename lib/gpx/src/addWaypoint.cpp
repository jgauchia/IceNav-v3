/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#include "addWaypoint.hpp"
#include "esp_log.h"

extern const uint8_t SD_CS;

File gpxFile;
wayPoint addWpt;
wayPoint loadWpt; 

static const char* TAG PROGMEM = "GPX Creation file";

/**
 * @brief Open GPX File exists if not, create new.
 * 
 * @param gpxFilename -> GPX file
 */
void createGpxFile(const char *gpxFilename)
{
  if (!SD.exists(wptFolder))
  {
    ESP_LOGI(TAG,"WPT folder not exists");
    if (SD.mkdir(wptFolder))
      ESP_LOGI(TAG, "WPT folder created");
    else
     log_i("WPT folder not created");
  }
  
  gpxFile = SD.open(gpxFilename, FILE_READ);

  if (!gpxFile)
  {
    log_i("GPX File not exists");
    gpxFile = SD.open(gpxFilename, FILE_WRITE);
    if (gpxFile)
    {
      log_i("Creating GPX File");
      gpxFile.println(gpxType.header);
      gpxFile.print(gpxType.footer);
      gpxFile.close();
    }
    else
      ESP_LOGE(TAG,"GPX File creation error");
  }
  else
  {
    log_i("GPX File exists");
    gpxFile.close();
  }
}