/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#include "addWaypoint.hpp"

wayPoint addWpt = {0, 0, 0, (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", 0, 0, 0, 0};
wayPoint loadWpt = {0, 0, 0, (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", 0, 0, 0, 0};
FILE *gpxFile;
extern Storage storage;
extern Gps gps;

/**
 * @brief Open GPX File exists if not, create new.
 * 
 * @param gpxFilename -> GPX file
 */
void openGpxFile(const char *gpxFilename)
{
  time_t localTime = time(NULL);

  if (!storage.exists(wptFolder))
  {
    log_i("WPT folder not exists");
    if (storage.mkdir(wptFolder))
    {
      storage.setFileTime(wptFolder, localTime);
      log_i("WPT folder created");
    }
    else
      log_i("WPT folder not created");
  }

  gpxFile = storage.open(gpxFilename, "r");

  if (!gpxFile)
  {
    log_i("GPX File not exists");
    gpxFile = storage.open(gpxFilename, "w");
    if (gpxFile)
    {
      log_i("Creating GPX File");
      storage.println(gpxFile, gpxType.header);
      storage.print(gpxFile, gpxType.footer);
      storage.close(gpxFile);
      storage.setFileTime(gpxFilename, localTime);
      log_i("File Size: %d", storage.size(gpxFilename));
    }
    else
      log_e("GPX File creation error");
  }
  else
  {
    log_i("GPX File exists");
    storage.close(gpxFile);
  }
}

/**
 * @brief Add waypoint to file.
 * 
 * @param gpxFilename -> GPX file
 * @param addWpt -> Waypoint
 */
void addWaypointToFile(const char *gpxFilename, wayPoint addWpt)
{
  // gpxFile = storage.open(gpxFilename, "w");

  // if (gpxFile)
  // {
  //   log_i("Append Waypoint to file");
  //   log_i("Name %s",addWpt.name);
  //   log_i("Lat %f",addWpt.lat);
  //   log_i("Lon %f",addWpt.lon);
  //   log_i("File Size: %d",storage.size(gpxFilename));
  //   storage.seek(gpxFile, storage.size(gpxFilename) - 6,SEEK_SET);

  //   storage.write(gpxFile,(uint8_t*)gpxWPT.open, sizeof(gpxWPT.open));
  //   sprintf(textFmt,wptType.lat,addWpt.lat);
  //   // gpxFile.println(textFmt);
  //   storage.write(gpxFile,textFmt,sizeof(textFmt));
  //   sprintf(textFmt,wptType.lon,addWpt.lon);
  //   // gpxFile.println(textFmt);
  //   storage.write(gpxFile,textFmt,sizeof(textFmt));
  //   sprintf(textFmt,wptType.name,addWpt.name);
  //   // gpxFile.println(textFmt);
  //   storage.write(gpxFile,textFmt,sizeof(textFmt));
  //   // gpxFile.println(gpxWPT.close);
  //   storage.write(gpxFile,gpxWPT.close,sizeof(gpxWPT.close));
  //   // gpxFile.print(gpxType.footer);
  //   storage.write(gpxFile,gpxType.footer,sizeof(gpxType.footer));
  //   storage.close(gpxFile);
  //   vTaskDelay(100);
  //   // gpxFile = storage.open(gpxFilename, "r");
  //   log_i("File Size: %d",storage.size(gpxFilename));
  // }
  // else
  //   log_e("Waypoint not append to file");
}
