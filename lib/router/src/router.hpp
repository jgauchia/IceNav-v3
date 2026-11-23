/**
 * @file router.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Public router interface — load graph and compute A* route
 * @version 0.3.3
 * @date 2026-10
 */

#pragma once
#include "routeReader.hpp"
#include "astar.hpp"
#include "globalGpxDef.h"

enum class RouterResult
{
    OK,
    NO_PATH,
    LOAD_ERROR,
};

class Router
{
public:
    RouterResult route(float src_lat, float src_lon,
                       float dst_lat, float dst_lon,
                       TrackVector& out_track);

private:
    RouteReader loader;
};

extern Router router;
