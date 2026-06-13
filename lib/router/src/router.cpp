/**
 * @file router.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Router implementation — combines GraphLoader and A*
 * @version 0.2.9
 * @date 2026-06
 */

#include "router.hpp"
#include "esp_timer.h"
#include "esp_log.h"
#include "settings.hpp"

static const char* TAG_ROUTER = "ROUTER";

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
    int64_t t_start = esp_timer_get_time();

    if (!loader_.isLoaded())
    {
        if (!loader_.load())
            return RouterResult::LOAD_ERROR;
    }

    loader_.preloadPoint(src_lat, src_lon);
    loader_.preloadPoint(dst_lat, dst_lon);

    uint32_t src_node = loader_.nearestNode(src_lat, src_lon);
    uint32_t dst_node = loader_.nearestNode(dst_lat, dst_lon);

    out_track = astarRoute(loader_, src_node, dst_node, (float)navSet.routeSpeed);

    int64_t t_end = esp_timer_get_time();
    int64_t elapsed_us = t_end - t_start;
    ESP_LOGE(TAG_ROUTER, "route (%.5f,%.5f)->(%.5f,%.5f): nodes %u->%u, %lld us (%lld ms), waypoints=%u",
             src_lat, src_lon, dst_lat, dst_lon,
             src_node, dst_node,
             elapsed_us, elapsed_us / 1000,
             (unsigned)out_track.size());

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
