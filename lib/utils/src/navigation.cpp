/**
 * @file navigation.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "navigation.hpp"

extern lv_obj_t *turnDistLabel; 
extern lv_obj_t *turnImg;

LV_IMG_DECLARE(straight);
LV_IMG_DECLARE(slleft);
LV_IMG_DECLARE(slright);
LV_IMG_DECLARE(tleft);
LV_IMG_DECLARE(tright);
LV_IMG_DECLARE(uleft);
LV_IMG_DECLARE(uright);
LV_IMG_DECLARE(finish);
LV_IMG_DECLARE(outtrack);

 /**
 * @brief Finds the closest track point index to the user's current position, with an adaptive search window.
 *
 * @details Searches for the closest waypoint in the track to the user's given latitude and longitude.
 *          To optimize performance, it initially searches within a window of points around the last known closest index (lastIdx).
 *          If the user is detected to be far from the last known point (e.g., due to a fast jump, simulation, or GPS error),
 *          the function automatically expands the search to cover the entire track, ensuring robust operation even with rapid movements.
 *          Additionally, to avoid small unwanted backward jumps (e.g., due to GPS noise), small regressions are ignored:
 *          if the closest point found is behind lastIdx but less than 5 points back, it keeps lastIdx as the result.
 *
 * @param userLat   Current latitude of the user.
 * @param userLon   Current longitude of the user.
 * @param track     The vector of wayPoints representing the full track.
 * @param lastIdx   The index of the last known closest point on the track.
 * @return          The index of the closest point in the track within the search window or full track if needed.
 */
int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx) 
{
    int window = 50;
    int n = (int)track.size();
    int start = std::max(0, lastIdx - window);
    int end = std::min(n - 1, lastIdx + window);

    // If the user has strayed far, search the entire track
    float minDist = calcDist(userLat, userLon, track[lastIdx].lat, track[lastIdx].lon);
    int closestIdx = lastIdx;

    // If the distance to the last point is large, search the entire track
    if (minDist > 2 * window)       
    { // Or a threshold in meters, e.g., 50m
        start = 0;
        end = n - 1;
    }

    for (int i = start; i <= end; ++i)
    {
        float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
        if (d < minDist)
        {
            minDist = d;
            closestIdx = i;
        }
    }
    // Prevent small backward jumps on the track
    if (closestIdx < lastIdx && (lastIdx - closestIdx) < 5) {
        closestIdx = lastIdx;
    }
    return closestIdx;
}

/**
 * @brief Simplified navigation logic that displays only the next significant event (straight, soft turn, or sharp turn),
 *        notifying the user when the event is within a specified warning distance.
 *
 * @details This function determines the user's position relative to a preloaded GPX track and identifies the next relevant turn,
 *          based on angular thresholds and proximity. When the upcoming turn is close enough, it displays the corresponding
 *          directional icon.
 *
 * @param userLat             Current latitude.
 * @param userLon             Current longitude.
 * @param userHeading         Current heading (in degrees).
 * @param speed_kmh           Current speed in km/h
 * @param track               Vector of GPX track waypoints.
 * @param turns               Vector of detected turn points (with indices and angles).
 * @param state               Navigation state, including last track index and next turn index.
 * @param minAngleForCurve    Minimum turn angle to classify as a curve (default is 15 degrees).
 * @param warnDist            Distance threshold (in meters) to trigger event notification (default is 100 meters).
 */
void updateNavigation(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const std::vector<wayPoint>& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist
)
{
    const float minTurnDist = 5.0f;  

    int closestIdx = findClosestTrackPoint(userLat, userLon, track, state.lastTrackIdx);
    float distToTrack = calcDist(userLat, userLon, track[closestIdx].lat, track[closestIdx].lon);
    if (distToTrack > 30.0) 
    {
        lv_img_set_src(turnImg, &outtrack);
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Advance the turnpoint index if it has already been passed (by index)
    while (state.nextTurnIdx < (int)turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;

    // Search for the next relevant event (the first one with sufficient distance)
    int nextEventIdx = -1;
    float distanceToNextEvent = std::numeric_limits<float>::max();
    float abs_angle = 0.0;
    bool derecha = false;

    for (int i = state.nextTurnIdx; i < (int)turns.size(); ++i)
    {
        float turnLat = track[turns[i].idx].lat;
        float turnLon = track[turns[i].idx].lon;
        float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);
        // If the event is too close (or already passed), skip it and continue searching
        if (distanceToTurn < minTurnDist)
            continue;

        distanceToNextEvent = distanceToTurn;
        nextEventIdx = i;
        abs_angle = fabs(turns[i].angle);
        derecha = (turns[i].angle > 0);
        break; // Found the next relevant event, exit the loop
    }

    // If there are no remaining relevant events
    if (nextEventIdx == -1) 
    {
        lv_img_set_src(turnImg, &finish);
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Display only when there are <= warnDist meters remaining
    if (distanceToNextEvent <= warnDist)
    {
        if (abs_angle >= minAngleForCurve && abs_angle < 60) 
        {
            if (derecha)
                lv_img_set_src(turnImg, &slright);
            else
                lv_img_set_src(turnImg, &slleft);
        }
        else if (abs_angle >= 60) 
        {
            if (derecha)
                lv_img_set_src(turnImg, &tright);
            else
                lv_img_set_src(turnImg, &tleft);

        }
        else 
        {
            lv_img_set_src(turnImg, &straight);     
        }
    }
    else
    {
        lv_img_set_src(turnImg, &straight);
    }

    int roundedDist = ((int)distanceToNextEvent / 5) * 5;
    lv_label_set_text_fmt(turnDistLabel, "%4d", roundedDist);

    state.lastTrackIdx = closestIdx;
}
