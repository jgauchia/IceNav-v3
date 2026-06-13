/**
 * @file astar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  A* routing algorithm with Haversine heuristic
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once
#include "graph_loader.hpp"
#include "globalGpxDef.h"

TrackVector astarRoute(const GraphLoader& graph, uint32_t src_node, uint32_t dst_node, float maxSpeedKmh = 130.0f);
