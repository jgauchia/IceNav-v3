/**
 * @file globalGpxDef.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Global GPX Variables
 * @version 0.2.0
 * @date 2025-04
 */


#ifndef GLOBALGPXDEF_H
#define GLOBALGPXDEF_H

#include <pgmspace.h>
#include <stdint.h>

static const char* wptFile PROGMEM = "/sdcard/WPT/waypoint.gpx";
static const char* wptFolder PROGMEM = "/sdcard/WPT";

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
 * @brief Waypoint Structure
 *
 */
struct wayPoint
{
  double    lat;
  double    lon;
  float     ele;
  char*     time;
  char*     name;
  char*     desc;
  char*     src;
  char*     sym;
  char*     type;
  uint8_t   sat;
  float     hdop;
  float     vdop;
  float     pdop;
};

/**
 * @Brief gpxType definition
 *
 */
static const struct 
{
  const char* header; 
  const char* footer; 
} gpxType PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<gpx\n"
                      " version=\"1.0\"\n"
                      " creator=\"IceNav\"\n"
                      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                      " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">"
                      , "</gpx>" };


#endif
