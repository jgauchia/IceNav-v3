/**
 * @file addWaypoint.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Add Waypoint functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef ADDWAYPOINT_HPP
#define ADDWAYPOINT_HPP

#include "SD.h"
#include "globalGpxDef.h"

extern wayPoint addWpt;

void openGpxFile(const char* gpxFilename);
void addWaypointToFile(const char* gpxFilename, wayPoint addWpt);

#endif
