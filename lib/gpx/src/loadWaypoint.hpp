/**
 * @file loadWaypoint.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Load Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-01
 */

#ifndef LOADWAYPOINT_HPP
#define LOADWAYPOINT_HPP

#include "globalGpxDef.h"

extern wayPoint loadWpt;

void loadWptFile(String wpt);

#endif