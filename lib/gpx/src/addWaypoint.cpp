/**
 * @file addWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#include "addWaypoint.hpp"

extern const uint8_t SD_CS;

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
  if (!SD.exists(wptFolder))
  {
    log_i("WPT folder not exists");
    if (SD.mkdir(wptFolder))
     log_i("WPT folder created");
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
      log_e("GPX File creation error");
  }
  else
  {
    log_i("GPX File exists");
    gpxFile.close();
  }
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

  gpxFile = SD.open(gpxFilename, FILE_WRITE);

  if (gpxFile)
  {
    log_i("Append Waypoint to file");
    log_i("Name %s",addWpt.name);
    log_i("Lat %f",addWpt.lat);
    log_i("Lon %f",addWpt.lon);
    gpxFile.seek(gpxFile.size() - 6);
    gpxFile.write((uint8_t*)gpxWPT.open, sizeof(gpxWPT.open));

    // Coords
    sprintf(textFmt,wptType.lat,addWpt.lat);
    gpxFile.println(textFmt);
    sprintf(textFmt,wptType.lon,addWpt.lon);
    gpxFile.println(textFmt);
    // Waypoint elevation
    sprintf(textFmt,wptType.ele,addWpt.ele);
    gpxFile.println(textFmt);
    // Waypoint time
    time_t tUTCwpt = time(NULL);
    struct tm UTCwpt_tm;
    struct tm *tmUTCwpt = gmtime_r(&tUTCwpt, &UTCwpt_tm);
    strftime(textFmt, sizeof(textFmt), " <time>%Y-%m-%dT%H:%M:%SZ</time>", tmUTCwpt);
    gpxFile.println(textFmt);
    // Waypoint Name
    sprintf(textFmt,wptType.name,addWpt.name);
    gpxFile.println(textFmt);
    // Waypoint source
    gpxFile.println(wptType.src);
    // Number satellites
    sprintf(textFmt,wptType.sat,addWpt.sat);
    gpxFile.println(textFmt);
    // HDOP, VDOP, PDOP
    sprintf(textFmt,wptType.hdop,addWpt.hdop);
    gpxFile.println(textFmt);
    sprintf(textFmt,wptType.vdop,addWpt.vdop);
    gpxFile.println(textFmt);
    sprintf(textFmt,wptType.pdop,addWpt.pdop);
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
}
