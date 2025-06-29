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
 * @brief Navigation state structure for turn-by-turn guidance.
 *
 * @details This struct stores the persistent state of the navigation logic during a GPX-based turn-by-turn session.
 * 
 *          Fields:
 *              - lastTrackIdx: Index of the last closest track point matched to the user's current position.
 *              - nextTurnIdx: Index of the next turn point in the list of detected turns.
 *              - warnedTurn: True if the final turn warning has already been issued for the upcoming turn.
 *              - warnedPreTurn: True if the pre-turn warning (early notification) has already been issued for the next turn.
 *              - warnedStraight: True if the "continue straight" indication has already been shown when no turn is imminent.
 *
 *          This structure is updated on each navigation loop to avoid repeating turn notifications and to manage the current navigation context.
 */
struct NavState 
{
    int lastTrackIdx = 0;
    int nextTurnIdx = 0;
    bool warnedTurn = false;
    bool warnedPreTurn = false;
    bool warnedStraight = false;
};

int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx);
void updateNavigation(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const std::vector<wayPoint>& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist
);