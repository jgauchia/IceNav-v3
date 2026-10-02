/**
 * @file astar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  A* routing algorithm with Haversine heuristic
 * @version 0.3.0
 * @date 2026-10
 */

#pragma once
#include "routeReader.hpp"
#include "globalGpxDef.h"

TrackVector astarRoute(const RouteReader& graph, uint32_t src_node, uint32_t dst_node, float maxSpeedKmh = 130.0f);
