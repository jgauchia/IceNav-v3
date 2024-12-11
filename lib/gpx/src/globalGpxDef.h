/**
 * @file globalGpxDef.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Global GPX Variables
 * @version 0.1.9
 * @date 2024-12
 */


#ifndef GLOBALGPXDEF_H
#define GLOBALGPXDEF_H

#include <pgmspace.h>
#include "FS.h"
#include <regex>

static const char* wptFile PROGMEM = "/WPT/waypoint.gpx";
static const char* wptFolder PROGMEM = "/WPT";

/**
 * @brief Waypoint action enum
 *
 */
enum wptAction_t
{
    WPT_NONE,
    WPT_ADD,
    WPT_LOAD,
    WPT_EDIT,
    WPT_DEL,
};

/**
 * @brief Waypoint Action
 *
 */
extern uint8_t wptAction;

/**
 * @brief Waypoint File Content for REGEX
 *
 */
extern std::string wptContent;

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
} gpxWPT PROGMEM = { "<wpt" , "</wpt>" };

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
} wptType PROGMEM = { " lat=\"%f\""
                     , "lon=\"%f\">"
                     , " <ele>%f</ele>"
                     , " <time>%s</time>"
                     , " <name>%s</name>"
                     , " <desc>%s</desc>"
                     , " <src>IceNav</src>"
                     , " <sym>%s</sym>"
                     , " <type>%s</type>"
                     , " <sat>%d</sat>"
                     , " <hdop>%f</hdop>"
                     , " <vdop>%f</vdop>"
                     , " <pdop>%f</pdop>" };

#endif
