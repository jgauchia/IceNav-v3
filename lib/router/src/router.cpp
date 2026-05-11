/**
 * @file router.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Router implementation — combines GraphLoader and A*
 * @version 0.2.6
 * @date 2026-05
 */

#include "router.hpp"

Router router;

/**
 * @brief Compute an A* route from source to destination coordinates.
 *
 * Loads the graph cells covering src and dst if not already loaded.
 * Finds the nearest graph node to each coordinate, runs A*, and returns
 * the resulting TrackVector. The caller is responsible for protecting
 * trackData with routeMutex before assigning the result.
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
    if (!loader_.isLoaded())
    {
        if (!loader_.load(src_lat, src_lon, dst_lat, dst_lon))
            return RouterResult::LOAD_ERROR;
        loader_.buildEdgeCache(src_lat, src_lon, dst_lat, dst_lon);
    }

    uint32_t src_node = loader_.nearestNode(src_lat, src_lon);
    uint32_t dst_node = loader_.nearestNode(dst_lat, dst_lon);

    out_track = astarRoute(loader_, src_node, dst_node);

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
