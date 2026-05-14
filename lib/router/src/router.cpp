/**
 * @file router.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Router implementation — combines GraphLoader and A*
 * @version 0.3.0
 * @date 2026-05
 */

#include "router.hpp"
#include <cmath>
#include "esp_timer.h"

Router router;

/**
 * @brief Compute an A* route from source to destination coordinates.
 *
 * Loads the graph index and preloads src↔dst bbox pages if not already loaded.
 * Finds the nearest graph node to each coordinate, runs A*, and returns
 * the resulting TrackVector.
 *
 * @param src_lat   Source latitude in degrees
 * @param src_lon   Source longitude in degrees
 * @param dst_lat   Destination latitude in degrees
 * @param dst_lon   Destination longitude in degrees
 * @param out_track Output TrackVector filled with route waypoints
 * @return RouterResult::OK on success, error code otherwise
 */
RouterResult Router::route(float src_lat, float src_lon,
                           float dst_lat, float dst_lon,
                           TrackVector& out_track)
{
    int64_t t0 = esp_timer_get_time();

    if (!loader_.isLoaded())
    {
        if (!loader_.load(src_lat, src_lon, dst_lat, dst_lon))
            return RouterResult::LOAD_ERROR;
    }

    int64_t t1 = esp_timer_get_time();
    printf("DEBUG ROUTER: load+preload = %lld ms\n", (t1 - t0) / 1000);

    uint32_t src_node = loader_.nearestNode(src_lat, src_lon);
    uint32_t dst_node = loader_.nearestNode(dst_lat, dst_lon);

    int64_t t2 = esp_timer_get_time();
    printf("DEBUG ROUTER: nearestNode x2 = %lld ms\n", (t2 - t1) / 1000);

    out_track = astarRoute(loader_, src_node, dst_node);

    int64_t t3 = esp_timer_get_time();
    printf("DEBUG ROUTER: astar = %lld ms\n", (t3 - t2) / 1000);

    if (out_track.empty())
        return RouterResult::NO_PATH;

    return RouterResult::OK;
}

/**
 * @brief Unload the graph from PSRAM and reset loader state.
 */
void Router::unload()
{
    loader_.unload();
}
