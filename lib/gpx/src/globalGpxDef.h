/**
 * @file globalGpxDef.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Global GPX Variables
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <pgmspace.h>
#include <stdint.h>

static const char* wptFile PROGMEM = "/sdcard/WPT/waypoint.gpx";
static const char* wptFolder PROGMEM = "/sdcard/WPT";
static const char* trkFolder PROGMEM = "/sdcard/TRK";

/**
 * @brief Waypoint action enum
 *
 */
enum gpxAction_t
{
    WPT_NONE,
    WPT_ADD,
    GPX_LOAD,
    GPX_EDIT,
    GPX_DEL,
};

/**
 * @brief Waypoint Action
 *
 */
extern uint8_t gpxAction;

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
 * @brief Track turn points structure
 *
 */
struct TurnPoint 
{
  int idx;            // Track IDX
  double angle;       // turn angle (positive = right, negative = left)
  double distance;    // distance from start (in meters)
};

/**
 * @brief Navigation state structure
 *
 */
struct NavState
{
  int lastTrackIdx = 0;
  int nextTurnIdx = 0;
  bool warnedTurn = false;
  bool warnedPreTurn = false;
  bool warnedStraight = false;
};

/**
 * @Brief GPX header file format
 *
 */
static const char* gpxHeader PROGMEM = { "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<gpx\n"
                      " version=\"1.0\"\n"
                      " creator=\"IceNav\"\n"
                      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                      " xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
                      " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n"
                      "</gpx>" };