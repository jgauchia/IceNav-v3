/**
 * @file addWaypoint.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Add Waypoint functions
 * @version 0.2.0_alpha
 * @date 2024-12
 */

#ifndef ADDWAYPOINT_HPP
#define ADDWAYPOINT_HPP

#include "SD.h"
#include "globalGpxDef.h"

extern wayPoint addWpt;

void openGpxFile(const char* gpxFilename);
void addWaypointToFile(const char* gpxFilename, wayPoint addWpt);

#endif
