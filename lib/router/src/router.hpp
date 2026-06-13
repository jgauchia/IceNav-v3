/**
 * @file router.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Public router interface — load graph and compute A* route
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once
#include "graph_loader.hpp"
#include "astar.hpp"
#include "globalGpxDef.h"

enum class RouterResult
{
    OK,
    NO_GRAPH_FILE,
    OUT_OF_MEMORY,
    NO_PATH,
    LOAD_ERROR,
};

class Router
{
public:
    RouterResult route(float src_lat, float src_lon,
                       float dst_lat, float dst_lon,
                       TrackVector& out_track);
    void unload();
    bool isLoaded() const { return loader_.isLoaded(); }

private:
    GraphLoader loader_;
};

extern Router router;
