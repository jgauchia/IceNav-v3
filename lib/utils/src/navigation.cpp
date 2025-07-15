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
 * @brief Finds the closest track point index to the user's current position using an adaptive local/global search.
 *
 * @details This function locates the closest waypoint in the GPX track to the current GPS coordinates.
 *          - Initially performs a localized search within a window around the last known closest index (`lastIdx`) to optimize performance.
 *          - If the distance to the closest point is too large (>30m) or the previous index is invalid, a full search of the track is triggered.
 *          - Prevents undesired small backward jumps due to GPS noise by ignoring points slightly behind the last index (<5 positions).
 *          - Returns the best matching index based on minimum haversine distance.
 *
 * @param userLat   Current latitude of the user.
 * @param userLon   Current longitude of the user.
 * @param track     The vector of wayPoints representing the full track.
 * @param lastIdx   Index of the last known closest point.
 * @return          Index of the closest waypoint found in the track.
 */
int findClosestTrackPoint(float userLat, float userLon, const std::vector<wayPoint>& track, int lastIdx)
{
    int n = (int)track.size();
    int window = 50;
    float minDist = std::numeric_limits<float>::max();
    int closestIdx = -1;

    // If last index is invalid or near track edges, force global search
    bool forceGlobal = lastIdx < 0 || lastIdx >= n;

    int start = forceGlobal ? 0 : std::max(0, lastIdx - window);
    int end   = forceGlobal ? n - 1 : std::min(n - 1, lastIdx + window);

    for (int i = start; i <= end; ++i) 
    {
        float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
        if (d < minDist) {
            minDist = d;
            closestIdx = i;
        }
    }

    // If user is far from last known point, perform full search
    if (!forceGlobal && minDist > 30.0f) 
    {
        for (int i = 0; i < n; ++i) {
            float d = calcDist(userLat, userLon, track[i].lat, track[i].lon);
            if (d < minDist) {
                minDist = d;
                closestIdx = i;
            }
        }
    }

    // Avoid small backward jumps (likely due to GPS noise)
    if (closestIdx < lastIdx && (lastIdx - closestIdx) < 5)
        return lastIdx;

    return closestIdx;
}

/**
 * @brief Updates turn-by-turn navigation status and on-screen instructions.
 *
 * @details Determines the user's current position relative to a GPX track and selects the next valid
 * navigation event (turn) from a list of preprocessed `TurnPoint` entries.
 *
 * The function handles:
 * - Real-time distance calculation to the track and the upcoming turn.
 * - Resynchronization in case of GPS drift, off-track situations, or user jumps.
 * - Prevention of regressions in track index to avoid flickering icons.
 * - Filtering of suspicious or invalid turns based on distance thresholds.
 * - Display of appropriate directional icons (left, right, straight, finish, out-of-track).
 *
 * Main logic steps:
 * - Use `findClosestTrackPoint()` to locate the closest point in the track to the current position.
 * - If off-track (>30m), mark the navigation state as off-route and restore `nextTurnIdx` using `lastValidTurnIdx`.
 * - Advance `nextTurnIdx` if the corresponding turn has been passed already.
 * - Skip suspiciously distant turn events (>2000m) to avoid invalid guidance.
 * - Classify turns by angular threshold into straight, soft, or hard turns.
 * - Show directional icon if within `warnDist`, or fallback to "straight ahead".
 * - Round distance to next event to the nearest 5 meters and update the display label.
 *
 * @param userLat             Current latitude.
 * @param userLon             Current longitude.
 * @param userHeading         Current heading (in degrees).
 * @param speed_kmh           Current speed in km/h.
 * @param track               Vector of GPX track waypoints.
 * @param turns               Vector of detected turn points (with indices and angles).
 * @param state               Persistent navigation state including current/last track and turn indices.
 * @param minAngleForCurve    Minimum angle (in degrees) to classify a soft curve (default: 15°).
 * @param warnDist            Distance threshold (in meters) to trigger the event warning (default: 100 m).
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

    // Handle off-track condition
    if (distToTrack > 30.0f) 
    {
        lv_img_set_src(turnImg, &outtrack);

        // Save current turn index if just detected off-track
        if (!state.isOffTrack) 
        {
            state.lastValidTurnIdx = state.nextTurnIdx;
            state.isOffTrack = true;
        }

        state.lastTrackIdx = closestIdx;
        return;
    }

    // Restore turn index if user returns to track
    if (state.isOffTrack)
    {
        state.nextTurnIdx = state.lastValidTurnIdx;
        state.isOffTrack = false;
    }

    // Advance nextTurnIdx if it's already passed
    while (state.nextTurnIdx < (int)turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;

    // No more turns remaining
    if (state.nextTurnIdx >= (int)turns.size()) 
    {
        lv_img_set_src(turnImg, &finish);
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Search for next valid event
    int nextEventIdx = -1;
    float distanceToNextEvent = std::numeric_limits<float>::max();
    float abs_angle = 0.0f;
    bool derecha = false;

    for (int i = state.nextTurnIdx; i < (int)turns.size(); ++i)
    {
        const float turnLat = track[turns[i].idx].lat;
        const float turnLon = track[turns[i].idx].lon;
        const float distanceToTurn = calcDist(userLat, userLon, turnLat, turnLon);

        if (distanceToTurn < minTurnDist)
            continue;

        if (distanceToTurn > 2000.0f) 
        {
            ESP_LOGW("NAV", "Skipping suspicious turn at index %d (dist=%.1f)", i, distanceToTurn);
            continue;
        }

        // Valid turn
        distanceToNextEvent = distanceToTurn;
        nextEventIdx = i;
        abs_angle = fabsf(turns[i].angle);
        derecha = (turns[i].angle > 0.0f);
        break;
    }

    if (nextEventIdx == -1) 
    {
        lv_img_set_src(turnImg, &finish);
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Display turn icon only within warning distance
    if (distanceToNextEvent <= warnDist)
    {
        if (abs_angle >= minAngleForCurve && abs_angle < 60.0f) 
        {
            lv_img_set_src(turnImg, derecha ? &slright : &slleft);
        }
        else if (abs_angle >= 60.0f) 
        {
            lv_img_set_src(turnImg, derecha ? &tright : &tleft);
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
