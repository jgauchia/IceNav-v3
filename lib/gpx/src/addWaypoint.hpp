/**
 * @file addWaypoint.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#ifndef ADDWAYPOINT_HPP
#define ADDWAYPOINT_HPP

#include "globalGpxDef.h"
#include "storage.hpp"
#include "settings.hpp"
#include "NMEAGPS.h"
#include "gps.hpp"
#include <sys/time.h>
#include <time.h>

extern wayPoint addWpt;

void openGpxFile(const char* gpxFilename);
void addWaypointToFile(const char* gpxFilename, wayPoint addWpt);

#endif
