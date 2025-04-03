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
  if (!storage.exists(wptFolder))
  {
    log_i("WPT folder not exists");
    if (storage.mkdir(wptFolder))
     log_i("WPT folder created");
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
  char textFmt[100] = "";
  gpxFile = storage.open(gpxFilename, "r+");

  if (gpxFile)
  {
    log_i("Append Waypoint to file");
    log_i("Name %s",addWpt.name);
    log_i("Lat %f",addWpt.lat);
    log_i("Lon %f",addWpt.lon);
    storage.seek(gpxFile, storage.size(gpxFilename) - 6,SEEK_SET);
    storage.print(gpxFile, gpxWPT.open);

    // Coords
    sprintf(textFmt,wptType.lat,addWpt.lat);
    storage.print(gpxFile,textFmt);
    sprintf(textFmt,wptType.lon,addWpt.lon);
    storage.println(gpxFile,textFmt);
    // Waypoint elevation
    sprintf(textFmt,wptType.ele,addWpt.ele);
    storage.println(gpxFile,textFmt);
    // Waypoint time
    time_t tUTCwpt = time(NULL);
    struct tm UTCwpt_tm;
    struct tm *tmUTCwpt = gmtime_r(&tUTCwpt, &UTCwpt_tm);
    strftime(textFmt, sizeof(textFmt), " <time>%Y-%m-%dT%H:%M:%SZ</time>", tmUTCwpt);
    storage.println(gpxFile,textFmt);
    // Waypoint Name
    sprintf(textFmt,wptType.name,addWpt.name);
    storage.println(gpxFile,textFmt);
    // Waypoint source
    storage.println(gpxFile,wptType.src);
    // Number satellites
    sprintf(textFmt,wptType.sat,addWpt.sat);
    storage.println(gpxFile,textFmt);
    // HDOP, VDOP, PDOP
    sprintf(textFmt,wptType.hdop,addWpt.hdop);
    storage.println(gpxFile,textFmt);
    sprintf(textFmt,wptType.vdop,addWpt.vdop);
    storage.println(gpxFile,textFmt);
    sprintf(textFmt,wptType.pdop,addWpt.pdop);
    storage.println(gpxFile,textFmt);

    storage.println(gpxFile,gpxWPT.close);
    storage.print(gpxFile,gpxType.footer);
    storage.close(gpxFile);
    log_i("File Size: %d",storage.size(gpxFilename));
  }
  else
    log_e("Waypoint not append to file");
}
