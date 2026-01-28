/**
 * @file navigation.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Navigation functions
 * @version 0.2.4
 * @date 2025-12
 */

#include "navigation.hpp"
#include <limits>
#include "esp_log.h"

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

// UI Throttling state
static const void* lastIconShown = nullptr;
static int lastDistShown = -1;

extern std::vector<TrackSegment> trackIndex;

/**
 * @brief Finds the closest track point index to the user's current position using an adaptive hierarchical search.
 *
 * @details Locates the closest waypoint in the GPX track to the current GPS coordinates.
 *          - Optimization: Uses Spatial Indexing and Squared Distance comparisons for O(log n) performance.
 *          - Performance: Operates on squared distances and radian-preconverted coordinates to minimize CPU overhead.
 *
 * @param userLat   Current latitude of the user (degrees).
 * @param userLon   Current longitude of the user (degrees).
 * @param track     The vector of wayPoints representing the full track.
 * @param lastIdx   Index of the last known closest point.
 * @param config    Navigation configuration parameters.
 * @return          Index of the closest waypoint found in the track.
 */
int findClosestTrackPoint(float userLat, float userLon, const TrackVector& track, int lastIdx, const NavConfig& config)
{
    int n = (int)track.size();
    float minDistSq = std::numeric_limits<float>::max();
    int closestIdx = -1;

    // Convert user coordinates to radians once for all loop iterations
    const float uLatRad = DEG2RAD(userLat);
    const float uLonRad = DEG2RAD(userLon);

    // Pre-calculate squared thresholds in angular units (radians^2) for performance
    // formula: angular_dist = metric_dist / EARTH_RADIUS
    const float invEarthRadius = 1.0f / EARTH_RADIUS;
    const float fastPathThresholdSq = (20.0f * invEarthRadius) * (20.0f * invEarthRadius);
    const float offTrackThresholdSq = (config.offTrackThreshold * invEarthRadius) * (config.offTrackThreshold * invEarthRadius);

    // Fast local search: checks a small window around the last known position
    if (lastIdx >= 0 && lastIdx < n)
    {
        int window = config.searchWindow;
        int start = std::max(0, lastIdx - 10); 
        int end   = std::min(n - 1, lastIdx + window);

        for (int i = start; i <= end; ++i) 
        {
            float dSq = calcDistSq(uLatRad, uLonRad, DEG2RAD(track[i].lat), DEG2RAD(track[i].lon));
            if (dSq < minDistSq) 
            {
                minDistSq = dSq;
                closestIdx = i;
            }
        }
        
        if (minDistSq < fastPathThresholdSq) 
             return closestIdx;
    }

    // Hierarchical Global Search: scans the TrackSegment index if local search fails
    if (closestIdx == -1 || minDistSq > offTrackThresholdSq) 
    {
        if (!trackIndex.empty())
        {
            minDistSq = std::numeric_limits<float>::max();
            closestIdx = -1;
            
            for (const auto& seg : trackIndex)
            {
                if (userLat <= seg.maxLat && userLat >= seg.minLat &&
                    userLon <= seg.maxLon && userLon >= seg.minLon)
                {
                    for (int i = seg.startIdx; i <= seg.endIdx; ++i)
                    {
                        float dSq = calcDistSq(uLatRad, uLonRad, DEG2RAD(track[i].lat), DEG2RAD(track[i].lon));
                        if (dSq < minDistSq)
                        {
                            minDistSq = dSq;
                            closestIdx = i;
                        }
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < n; ++i) 
            {
                float dSq = calcDistSq(uLatRad, uLonRad, DEG2RAD(track[i].lat), DEG2RAD(track[i].lon));
                if (dSq < minDistSq) 
                {
                    minDistSq = dSq;
                    closestIdx = i;
                }
            }
        }
    }
    
    if (closestIdx == -1) return std::max(0, lastIdx);

    if (closestIdx < lastIdx && (lastIdx - closestIdx) < config.maxBackwardJump)
        return lastIdx;

    return closestIdx;
}

/**
 * @brief Handle off-track condition and update navigation state
 *
 * @details Manages the navigation state when the user is off-track (>30m from track).
 *          Saves the current turn index and sets off-track flag.
 *
 * @param distToTrack Distance to the closest track point
 * @param state Navigation state to update
 * @param closestIdx Index of the closest track point
 */
void handleOffTrackCondition(float distToTrack, NavState& state, int closestIdx, const NavConfig& config)
{
    if (distToTrack > config.offTrackThreshold) 
    {
        if (lastIconShown != &outtrack)
        {
            lv_img_set_src(turnImg, &outtrack);
            lastIconShown = &outtrack;
        }

        // Save current turn index if just detected off-track
        if (!state.isOffTrack) 
        {
            state.lastValidTurnIdx = state.nextTurnIdx;
            state.isOffTrack = true;
        }

        state.lastTrackIdx = closestIdx;
    }
}

/**
 * @brief Advance turn index if turns have been passed
 *
 * @details Advances the nextTurnIdx if the corresponding turn has been passed already.
 *
 * @param turns Vector of detected turn points
 * @param state Navigation state to update
 * @param closestIdx Index of the closest track point
 */
void advanceTurnIndex(const std::vector<TurnPoint>& turns, NavState& state, int closestIdx)
{
    // Advance nextTurnIdx if it's already passed
    while (state.nextTurnIdx < turns.size() && turns[state.nextTurnIdx].idx <= closestIdx)
        state.nextTurnIdx++;
}

/**
 * @brief Find the next valid turn point
 *
 * @details Searches for the next valid turn point, skipping suspiciously distant ones.
 *
 * @param track Vector of track waypoints
 * @param turns Vector of detected turn points
 * @param userLat Current user latitude
 * @param userLon Current user longitude
 * @param state Navigation state
 * @return Index of the next valid turn, or -1 if none found
 */
int findNextValidTurn(const TrackVector& track, const std::vector<TurnPoint>& turns, 
                      float userLat, float userLon, int closestIdx, NavState& state, const NavConfig& config)
{
    for (int i = state.nextTurnIdx; i < turns.size(); ++i)
    {
        // Only skip turns that are physically behind our current track index
        if (turns[i].idx <= closestIdx)
            continue;

        return i; // Found the next logical turn
    }
    
    return -1; // Truly no more turns
}

/**
 * @brief Updates turn-by-turn navigation status and on-screen instructions.


/**
 * @brief Calculates the closest point on a line segment to a given point.
 *
 * @details Projects the user coordinates onto the segment formed by two track points (A and B).
 *          - Geographic Scaling: Applies a cosine latitude factor for accurate projection on Earth's surface.
 *          - Performance: Uses pre-calculated squared distances and radian coordinates.
 *
 * @param pLat User latitude.
 * @param pLon User longitude.
 * @param aLat Segment start latitude.
 * @param aLon Segment start longitude.
 * @param bLat Segment end latitude.
 * @param bLon Segment end longitude.
 * @param outLat Output for projected latitude.
 * @param outLon Output for projected longitude.
 * @return Squared distance to the segment in meters^2.
 */
float projectOnSegment(float pLat, float pLon, float aLat, float aLon, float bLat, float bLon, float& outLat, float& outLon)
{
    // Apply cosine factor to longitude for correct spatial projection
    float cosFactor = cosf(DEG2RAD((aLat + bLat) / 2.0f));
    
    float dLat = bLat - aLat;
    float dLon = (bLon - aLon) * cosFactor;
    float pLatRel = pLat - aLat;
    float pLonRel = (pLon - aLon) * cosFactor;

    float denom = dLat * dLat + dLon * dLon;
    if (denom == 0)
    {
        outLat = aLat;
        outLon = aLon;
        return calcDistSq(DEG2RAD(pLat), DEG2RAD(pLon), DEG2RAD(aLat), DEG2RAD(aLon));
    }

    // Projection factor t using scaled coordinates for geographic accuracy
    float t = (pLatRel * dLat + pLonRel * dLon) / denom;
    
    // Clamp t to [0, 1] to stay within the segment
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    outLat = aLat + t * (bLat - aLat);
    outLon = aLon + t * (bLon - aLon);

    return calcDistSq(DEG2RAD(pLat), DEG2RAD(pLon), DEG2RAD(outLat), DEG2RAD(outLon));
}

/**
 * @brief Updates turn-by-turn navigation status and on-screen instructions.
 *
 * @details Determines the user's current position relative to a GPX track and selects the next valid
 * navigation event (turn) from a list of preprocessed `TurnPoint` entries.
 *
 * Main logic steps:
 * - Use `findClosestTrackPoint()` to locate the closest point in the track to the current position.
 * - Project user position onto adjacent segments for smooth coordinate tracking and accurate distance.
 * - Handles off-track condition with a squared distance threshold.
 * - Advances turn index and updates directional icons based on Euclidean distance to upcoming events.
 *
 * @param userLat             Current latitude (degrees).
 * @param userLon             Current longitude (degrees).
 * @param userHeading         Current heading (in degrees).
 * @param speed_kmh           Current speed in km/h.
 * @param track               Vector of GPX track waypoints.
 * @param turns               Vector of detected turn points (with indices and angles).
 * @param state               Persistent navigation state including current/last track and turn indices.
 * @param minAngleForCurve    Minimum angle (in degrees) to classify a soft curve (default: 15°).
 * @param warnDist            Distance threshold (in meters) to trigger the event warning (default: 100 m).
 * @param config              Navigation configuration parameters.
 */
void updateNavigation(
    float userLat, float userLon, float userHeading, float speed_kmh,
    const TrackVector& track,
    const std::vector<TurnPoint>& turns,
    NavState& state,
    float minAngleForCurve,
    float warnDist,
    const NavConfig& config
)
{
    int closestIdx = findClosestTrackPoint(userLat, userLon, track, state.lastTrackIdx, config);
    
    // Convert to radians for optimized projection calculation
    const float uLatRad = DEG2RAD(userLat);
    const float uLonRad = DEG2RAD(userLon);

    // Projection on segments for smooth tracking
    float pLat = track[closestIdx].lat;
    float pLon = track[closestIdx].lon;

    // Check segments around closestIdx to find the real projection
    float bestLat = pLat, bestLon = pLon;
    float minDistSq = calcDistSq(uLatRad, uLonRad, DEG2RAD(pLat), DEG2RAD(pLon));

    // Check previous segment
    if (closestIdx > 0)
    {
        float tLat, tLon;
        float dSq = projectOnSegment(userLat, userLon, track[closestIdx - 1].lat, track[closestIdx - 1].lon, 
                                   track[closestIdx].lat, track[closestIdx].lon, tLat, tLon);
        if (dSq < minDistSq)
        {
            minDistSq = dSq;
            bestLat = tLat;
            bestLon = tLon;
        }
    }

    // Check next segment
    if (closestIdx < track.size() - 1)
    {
        float tLat, tLon;
        float dSq = projectOnSegment(userLat, userLon, track[closestIdx].lat, track[closestIdx].lon, 
                                   track[closestIdx + 1].lat, track[closestIdx + 1].lon, tLat, tLon);
        if (dSq < minDistSq)
        {
            minDistSq = dSq;
            bestLat = tLat;
            bestLon = tLon;
        }
    }

    float distToTrack = sqrtf(minDistSq) * EARTH_RADIUS; 
    state.projLat = bestLat;
    state.projLon = bestLon;

    // Handle off-track condition
    if (distToTrack > config.offTrackThreshold) 
    {
        handleOffTrackCondition(distToTrack, state, closestIdx, config);
        return;
    }

    // Restore turn index if user returns to track
    if (state.isOffTrack)
    {
        state.nextTurnIdx = state.lastValidTurnIdx;
        state.isOffTrack = false;
        // Reset trackers to force update after state change
        lastIconShown = nullptr;
        lastDistShown = -1;
    }

    // Advance turn index if turns have been passed
    advanceTurnIndex(turns, state, closestIdx);

    // No more turns remaining
    if (state.nextTurnIdx >= turns.size()) 
    {
        if (lastIconShown != &finish)
        {
            lv_img_set_src(turnImg, &finish);
            lastIconShown = &finish;
        }
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Find next valid turn
    int nextEventIdx = findNextValidTurn(track, turns, userLat, userLon, closestIdx, state, config);
    
    if (nextEventIdx == -1) 
    {
        if (lastIconShown != &finish)
        {
            lv_img_set_src(turnImg, &finish);
            lastIconShown = &finish;
        }
        state.lastTrackIdx = closestIdx;
        return;
    }

    // Calculate turn details
    const float turnLat = track[turns[nextEventIdx].idx].lat;
    const float turnLon = track[turns[nextEventIdx].idx].lon;
    const float distanceToNextEvent = calcDist(userLat, userLon, turnLat, turnLon);
    const float abs_angle = fabsf(turns[nextEventIdx].angle);
    const bool isRight = (turns[nextEventIdx].angle > 0.0f);

    // Determine appropriate turn icon
    const void* currentIcon = &straight;
    if (distanceToNextEvent <= warnDist)
    {
        if (abs_angle >= minAngleForCurve && abs_angle < 60.0f) 
            currentIcon = isRight ? &slright : &slleft;
        else if (abs_angle >= 60.0f) 
            currentIcon = isRight ? &tright : &tleft;
    }

    if (currentIcon != lastIconShown)
    {
        lv_img_set_src(turnImg, currentIcon);
        lastIconShown = currentIcon;
    }

    // Update distance label only if rounded value changes
    int roundedDist = ((int)distanceToNextEvent / 5) * 5;
    if (roundedDist != lastDistShown)
    {
        lv_label_set_text_fmt(turnDistLabel, "%4d", roundedDist);
        lastDistShown = roundedDist;
    }

    state.lastTrackIdx = closestIdx;
}
