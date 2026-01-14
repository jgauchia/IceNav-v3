/**
 * @file navigation.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <stdint.h>
#include <vector>
#include "globalGpxDef.h"
#include "gpsMath.hpp"
#include "lvgl.h"

/**
 * @brief Persistent navigation state for turn-by-turn guidance.
 *
 * @details This structure maintains the state of the navigation system during GPX-based turn-by-turn routing.
 *          It tracks both current and upcoming turn indices and handles off-track situations gracefully.
 *          
 *          Fields:
 *          - lastTrackIdx: Index of the last closest track point matched to the user's current position.
 *          - nextTurnIdx: Index of the next turn point in the list of detected turns.
 *          - lastValidTurnIdx: Backup index of the last valid nextTurnIdx before going off-track, used to restore state.
 *          - isOffTrack: Flag indicating whether the user is currently considered off the track.
 *
 *          This structure should persist across navigation updates to manage context and prevent redundant alerts.
 */
/**
 * @brief Navigation configuration parameters
 *
 * @details Contains configurable parameters for navigation behavior.
 *          These values were previously hardcoded and are now configurable.
 */
struct NavConfig 
{
    int searchWindow = 100;          /**< Window size for local search in findClosestTrackPoint */
    float offTrackThreshold = 50.0f; /**< Distance threshold for off-track detection (meters) */
    float minTurnDistance = 5.0f;    /**< Minimum distance for valid turn detection (meters) */
    float maxTurnDistance = 2000.0f; /**< Maximum distance for suspicious turn filtering (meters) */
    int maxBackwardJump = 8;         /**< Maximum backward positions to prevent GPS noise jumps */
};

struct NavState 
{
    int lastTrackIdx = 0;
    int nextTurnIdx = 0;
    int lastValidTurnIdx = 0;  
    bool isOffTrack = false;   
};

int findClosestTrackPoint(float userLat, float userLon, const TrackVector& track, int lastIdx, const NavConfig& config = NavConfig{});
void handleOffTrackCondition(float distToTrack, NavState& state, int closestIdx, const NavConfig& config = NavConfig{});
void advanceTurnIndex(const std::vector<TurnPoint>& turns, NavState& state, int closestIdx);
int findNextValidTurn(const TrackVector& track, const std::vector<TurnPoint>& turns, 
                      float userLat, float userLon, NavState& state, const NavConfig& config = NavConfig{});
void displayTurnIcon(float distanceToNextEvent, float abs_angle, bool derecha, float warnDist, float minAngleForCurve);
void updateNavigation
(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const TrackVector& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist,
    const NavConfig& config = NavConfig{}
);