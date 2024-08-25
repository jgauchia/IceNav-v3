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
  adquireSdSPI();

  gpxFile = SD.open(gpxFilename, FILE_READ);

  if (!gpxFile)
  {
    log_i("GPX File not exists");
    gpxFile = SD.open(gpxFilename, FILE_WRITE);
    if (gpxFile)
    {
      log_i("Creating GPX File");
      gpxFile.println(gpxType.header);
      gpxFile.println(gpxType.footer);
      gpxFile.close();
    }
    else
      log_e("GPX File creation error");
  }
  else
  {
    log_i("GPX File exists");
    gpxFile.close();
  }

  releaseSdSPI();
}
