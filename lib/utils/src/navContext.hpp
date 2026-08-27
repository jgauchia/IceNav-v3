/**
 * @file navContext.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Shared navigation state aggregate
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#include <atomic>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "navigation.hpp"
#include "climbAnalyzer.hpp"

/**
 * @brief Shared navigation state aggregate
 *
 * @details Groups the loaded track, its spatial index, detected turn points, climb profile,
 *          navigation progress and A* routing request state. routeMutex protects trackData
 *          and its derived members while the router task swaps in a new route.
 */
struct NavContext
{
    TrackVector trackData;                  /**< Loaded track/route waypoints (PSRAM) */
    std::vector<TrackSegment> trackIndex;   /**< Spatial index of track segments */
    std::vector<TurnPoint> turnPoints;      /**< Detected turn points along the track */
    ClimbAnalyzer climbAnalyzer;            /**< Climb profile analyzer */
    NavState navState;                      /**< Navigation progress state */
    float routeDstLat = 0.0f;               /**< Router destination latitude */
    float routeDstLon = 0.0f;               /**< Router destination longitude */
    std::atomic<bool> rerouteRequested {false}; /**< Flag to trigger A* route calculation */
    std::atomic<bool> wptNavActive {false};     /**< Waypoint navigation active: auto-reroute on deviation applies only to routed waypoint destinations */
    SemaphoreHandle_t routeMutex = nullptr; /**< Mutex protecting trackData during route updates */
};

extern NavContext navCtx;
