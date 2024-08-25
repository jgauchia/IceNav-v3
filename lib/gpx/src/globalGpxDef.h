/**
 * @file globalGpxDef.h
 * @brief  Global GPX Variables
 * @version 0.1.8
 * @date 2024-06
 */


#ifndef GLOBALWPTDEF_H
#define GLOBALWPTDEF_H

#include <pgmspace.h>
#include "FS.h"

/** 
 * @Brief GPX File
 *
 */

extern File gpxFile;

/**
 * @brief Waypoint Structure
 *
 */
struct wayPoint
{
  double lat;
  double lon;
  float  ele;
  char*  time;
  char*  name;
  char*  desc;
  char*  src;
  char*  sym;
  char*  type;
  int    sat;
  float  hdop;
  float  vdop;
  float  pdop;
};

/**
 * @Brief gpxType definition
 *
 */
static const struct 
{
  const char* header; 
  const char* footer; 
} gpxType PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
                      "<gpx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                      "xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\" "
                      "creator=\"IceNav\" "
                      "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">"
                      , "</gpx>" };

/**
 * @Brief WPT label definition
 *
 */
static const struct
{
  const char* open;
  const char* close;
} gpxWPT PROGMEM = { "\t<wpt " , "\t</wpt>\r\n" };

/**
 * @Brief wptType definition
 *
 */
static const struct
{
  const char* lat;
  const char* lon;
  const char* ele;
  const char* time;
  const char* name;
  const char* desc;
  const char* src;
  const char* sym;
  const char* type;
  const char* sat;
  const char* hdop;
  const char* vdop;
  const char* pdop;
} wptType PROGMEM = { "lat=\"%f\"\r\n"
                     , "lon=\"%f\"\r\n"
                     , "\t\t<ele>%f</ele>\r\n"
                     , "\t\t<time>%s</time>\r\n"
                     , "\t\t<name>%s</name>\r\n"
                     , "\t\t<desc>%s</desc>\r\n"
                     , "\t\t<src>IceNav</src>\r\n"
                     , "\t\t<sym>%s</sym>\r\n"
                     , "\t\t<type>%s</type>\r\n"
                     , "\t\t<sat>%d</sat>\r\n"
                     , "\t\t<hdop>%f</hdop>\r\n"
                     , "\t\t<vdop>%f</vdop>\r\n"
                     , "\t\t<pdop>%f</pdop>\r\n" };

#endif
