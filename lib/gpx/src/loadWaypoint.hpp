/**
 * @file loadWaypoint.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Load Waypoint functions
 * @version 0.1.9_alpha
 * @date 2024-11
 */

#ifndef LOADWAYPOINT_HPP
#define LOADWAYPOINT_HPP

#include "globalGpxDef.h"

extern wayPoint loadWpt;

void loadWptFile(String wpt);

#endif