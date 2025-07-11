/**
 * @file navigation.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.3
 * @date 2025-06
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
 *          - warnedTurn: Flag indicating whether the final turn warning has been issued.
 *          - warnedPreTurn: Flag indicating whether the early/pre-turn warning has been shown.
 *          - warnedStraight: Flag to prevent repeated "continue straight" indications.
 *          - lastValidTurnIdx: Backup index of the last valid nextTurnIdx before going off-track, used to restore state.
 *          - isOffTrack: Flag indicating whether the user is currently considered off the track.
 *
 *          This structure should persist across navigation updates to manage context and prevent redundant alerts.
 */
struct NavState 
{
    int lastTrackIdx = 0;
    int nextTurnIdx = 0;
    bool warnedTurn = false;
    bool warnedPreTurn = false;
    bool warnedStraight = false;
    int lastValidTurnIdx = 0;  
    bool isOffTrack = false;   
};

int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx);
void updateNavigation
(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const std::vector<wayPoint>& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist
);