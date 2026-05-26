/**
 * @file astar.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  A* routing algorithm with Haversine heuristic
 * @version 0.2.7
 * @date 2026-05
 */

#pragma once
#include "graph_loader.hpp"
#include "globalGpxDef.h"

TrackVector astarRoute(const GraphLoader& graph, uint32_t src_node, uint32_t dst_node);
