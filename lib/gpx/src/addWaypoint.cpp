/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Add Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "addWaypoint.hpp"

extern const int SD_CS;

wayPoint addWpt = {0, 0, 0, (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", 0, 0, 0, 0};
wayPoint loadWpt = {0, 0, 0, (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", 0, 0, 0, 0};
File gpxFile;

/**
 * @brief Open GPX File exists if not, create new.
 * 
 * @param gpxFilename -> GPX file
 */
void openGpxFile(const char* gpxFilename)
{
  acquireSdSPI();

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
      log_e("GPX File creation error");
  }
  else
  {
    log_i("GPX File exists");
    gpxFile.close();
  }

  releaseSdSPI();
}

/**
 * @brief Add waypoint to file.
 * 
 * @param gpxFilename -> GPX file
 * @param addWpt -> Waypoint
 */
void addWaypointToFile(const char* gpxFilename, wayPoint addWpt)
{
  char textFmt[100] = "";

  acquireSdSPI();

  gpxFile = SD.open(gpxFilename, FILE_WRITE);

  if (gpxFile)
  {
    log_i("Append Waypoint to file");
    log_i("Name %s",addWpt.name);
    log_i("Lat %f",addWpt.lat);
    log_i("Lon %f",addWpt.lon);
    gpxFile.seek(gpxFile.size() - 6);
    gpxFile.write((uint8_t*)gpxWPT.open, sizeof(gpxWPT.open));
    sprintf(textFmt,wptType.lat,addWpt.lat);
    gpxFile.println(textFmt);
    sprintf(textFmt,wptType.lon,addWpt.lon);
    gpxFile.println(textFmt);
    sprintf(textFmt,wptType.name,addWpt.name);
    gpxFile.println(textFmt);
    gpxFile.println(gpxWPT.close);
    gpxFile.print(gpxType.footer);
    gpxFile.close();
    vTaskDelay(100);
    gpxFile = SD.open(gpxFilename, FILE_READ);
    log_i("File Size: %d",gpxFile.size());
  }
  else
    log_e("Waypoint not append to file");

  releaseSdSPI();
}
