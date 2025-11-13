#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "globalGpxDef.h"
#include "storage.hpp"
#include "gps.hpp"
#include "tinyxml2.h"

static const char* TAGGPX PROGMEM = "GPX Tracker";

static const char* gpxTrackTag PROGMEM    = "trk";   /**< GPX track tag. */
static const char* gpxTrackSegmentTag PROGMEM = "trkseg";/**< GPX track segment tag. */
static const char* gpxTrackPointTag PROGMEM = "trkpt";/**< GPX track point tag. */
static const char* gpxLatElem PROGMEM     = "lat";   /**< GPX latitude attribute. */
static const char* gpxLonElem PROGMEM     = "lon";   /**< GPX longitude attribute. */
static const char* gpxEleElem PROGMEM     = "ele";   /**< GPX elevation element. */

static const char* gpxExtensionTag PROGMEM = "extensions"; /**< GPX extensions tag. */
static const char* gpxTrackPointExtensionTag PROGMEM = "gpxtpx:TrackPointExtension"; /**< GPX track point extension tag. */
static const char* gpxTemperatureElem PROGMEM = "gpxtpx:atemp"; /**< GPX temperature element. */

void startTrack();
void createTrackPoint();
void stopTrack();
std::string formatFloat(float value, int precision);

bool isTracking = false;