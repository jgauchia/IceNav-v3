/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Add Waypoint functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "addWaypoint.hpp"

extern const int SD_CS;

wayPoint addWpt = {0, 0, 0, "", "", "", "", "", "", 0, 0, 0, 0};
File gpxFile;

/**
 * @brief Open GPX File exists if not, create new.
 * 
 * @param gpxFilename -> GPX file
 */
void openGpxFile(const char* gpxFilename)
{
  #ifdef SPI_SHARED
  tft.waitDisplay();
  tft.endTransaction();
  digitalWrite(TFT_SPI_CS, HIGH);
  digitalWrite(SD_CS, LOW);
  #endif

  gpxFile = SD.open(gpxFilename, FILE_READ);

  if (!gpxFile)
    log_v("GPX no existe");
  else
    log_v("GPX existe");

  #ifdef SPI_SHARED
  digitalWrite(SD_CS, HIGH);
  digitalWrite(TFT_SPI_CS, LOW);
  tft.beginTransaction();
  #endif 
}
