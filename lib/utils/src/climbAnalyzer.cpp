/**
 * @file climbAnalyzer.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Climb profile analysis from loaded GPX track
 * @version 0.2.9
 * @date 2026-06
 */

#include "climbAnalyzer.hpp"
#include "navigation.hpp"
#include "lvgl.h"

const int triMask[TRI_ROWS] = { 9, 7, 5, 3, 1 };

extern lv_subject_t subject_climb_active;
extern lv_subject_t subject_climb_dist;
extern lv_subject_t subject_climb_gain;
extern lv_subject_t subject_climb_grade;
extern lv_subject_t subject_climb_idx;
extern lv_subject_t subject_climb_seg;
extern lv_subject_t subject_climb_total;
extern lv_subject_t subject_climb_cat;
extern lv_subject_t subject_climb_avg_grade;
extern lv_subject_t subject_climb_total_dist;
extern lv_subject_t subject_climb_total_gain;
extern lv_subject_t subject_climb_approaching;

/**
 * @brief Apply a simple moving-average smoothing to the raw elevation array.
 *
 * @details Reduces GPS elevation noise that would otherwise fragment climbs into
 *          dozens of tiny ascending/descending micro-segments.
 *
 * @param trackData Source track.
 * @param smoothed  Output vector of smoothed elevations (same size as trackData).
 * @param half      Half-window size; full window = 2*half+1 points.
 */
static void smoothElevation(const TrackVector& trackData, std::vector<float>& smoothed, int half)
{
    size_t n = trackData.size();
    smoothed.resize(n);
    for (size_t i = 0; i < n; ++i)
    {
        size_t a = (i > (size_t)half) ? i - half : 0;
        size_t b = (i + half + 1 < n) ? i + half + 1 : n;
        float sum = 0.0f;
        for (size_t j = a; j < b; ++j)
            sum += trackData[j].ele;
        smoothed[i] = sum / (float)(b - a);
    }
}

/**
 * @brief Scan the track and populate segments_ with qualifying climbs.
 *
 * @details Elevation is smoothed (window=15) before analysis to remove GPS noise
 *          that would otherwise fragment real climbs into sub-threshold fragments.
 *          A climb starts when the smoothed grade is positive and ends when it
 *          drops to zero or negative. Only segments with horizontal distance
 *          >= CLIMB_MIN_DIST_M and average grade >= CLIMB_MIN_GRADE are kept.
 *
 * @param trackData Loaded track points with ele and accumDist populated.
 */
void ClimbAnalyzer::analyze(const TrackVector& trackData, int startOffset)
{
    segments_.clear();
    if (trackData.size() < 2)
        return;

    std::vector<float> smoothed;
    smoothElevation(trackData, smoothed, 7);  // window = 15 points

    int startIdx = -1;
    float gainAcc = 0.0f;

    for (size_t i = 1; i < trackData.size(); ++i)
    {
        float dDist = trackData[i].accumDist - trackData[i - 1].accumDist;
        float dEle  = smoothed[i] - smoothed[i - 1];

        bool ascending = (dEle > 0.0f && dDist > 0.0f);

        if (ascending)
        {
            if (startIdx < 0)
            {
                startIdx = (int)(i - 1);
                gainAcc  = 0.0f;
            }
            gainAcc += dEle;
        }
        else
        {
            if (startIdx >= 0)
            {
                float segDist = trackData[i - 1].accumDist - trackData[startIdx].accumDist;
                float avgGrade = (segDist > 0.0f) ? (gainAcc / segDist * 100.0f) : 0.0f;

                if (segDist >= CLIMB_MIN_DIST_M && avgGrade >= CLIMB_MIN_GRADE)
                {
                    ClimbSegment seg;
                    seg.startIdx  = startOffset + startIdx;
                    seg.endIdx    = startOffset + (int)(i - 1);
                    seg.totalDist = segDist;
                    seg.totalGain = gainAcc;
                    seg.avgGrade  = avgGrade;
                    segments_.push_back(seg);
                }
                startIdx = -1;
                gainAcc  = 0.0f;
            }
        }
    }

    // Close any open climb that reaches the end of track
    if (startIdx >= 0)
    {
        int lastIdx = (int)trackData.size() - 1;
        float segDist = trackData[lastIdx].accumDist - trackData[startIdx].accumDist;
        float segGain = smoothed[lastIdx] - smoothed[startIdx];
        float avgGrade = (segDist > 0.0f) ? (segGain / segDist * 100.0f) : 0.0f;
        if (segDist >= CLIMB_MIN_DIST_M && avgGrade >= CLIMB_MIN_GRADE)
        {
            ClimbSegment seg;
            seg.startIdx  = startOffset + startIdx;
            seg.endIdx    = startOffset + lastIdx;
            seg.totalDist = segDist;
            seg.totalGain = segGain;
            seg.avgGrade  = avgGrade;
            segments_.push_back(seg);
        }
    }
}

/**
 * @brief Compute the UCI climb category from score (dist_m * grade_pct^2 / 100).
 *
 * @details Score thresholds: HC>=8000, CAT1>=3500, CAT2>=1500, CAT3>=800, CAT4<800.
 *
 * @param totalDist Total horizontal distance of the climb segment (meters).
 * @param avgGrade  Average grade of the climb segment (percent).
 * @return int  1=HC, 2=CAT1, 3=CAT2, 4=CAT3, 5=CAT4.
 */
int climbCategory(float totalDist, float avgGrade)
{
    float score = totalDist * avgGrade * avgGrade / 100.0f;
    if (score >= 8000.0f)
        return 1;
    if (score >= 3500.0f)
        return 2;
    if (score >= 1500.0f)
        return 3;
    if (score >= 800.0f)
        return 4;
    return 5;
}

/**
 * @brief Return packed RGB888 color for a given local grade value.
 *
 * @param avgGrade Local grade in percent (>= 0).
 * @return uint32_t  0xRRGGBB: red >=9%, orange >=6%, yellow >=3%, green otherwise.
 */
uint32_t climbSegmentColor(float avgGrade)
{
    if (avgGrade >= 9.0f)
        return 0xFF0000u;
    if (avgGrade >= 6.0f)
        return 0xFF6600u;
    if (avgGrade >= 3.0f)
        return 0xFFAA00u;
    return 0x00C800u;
}

/**
 * @brief Compute the canvas row for a given elevation value.
 *
 * @details Maps ele into [TRI_MARGIN, H-1] with the profile origin at the bottom.
 *
 * @param ele      Elevation to map (meters).
 * @param minEle   Minimum elevation of the visible window (meters).
 * @param eleRange Elevation span of the visible window (meters, >= 1).
 * @param H        Canvas height in pixels.
 * @return int     Canvas row index in [TRI_MARGIN, H-1].
 */
int calcYTop(float ele, float minEle, float eleRange, int H)
{
    int y = TRI_MARGIN + (int)((1.0f - (ele - minEle) / eleRange) * (H - 1 - TRI_MARGIN));
    if (y < TRI_MARGIN)
        y = TRI_MARGIN;
    if (y >= H)
        y = H - 1;
    return y;
}

/**
 * @brief Reset all state including detected segments and position tracking.
 */
void ClimbAnalyzer::clear()
{
    segments_.clear();
    activeSegIdx_ = -1;
    lastIdx_      = -1;
    prevSegIdx_   = -1;
}

/**
 * @brief Update climb subjects from current GPS position.
 *
 * @details Runs on Core 1 via the map async callback. In simulation mode uses
 *          simIndex directly; in real GPS mode calls findClosestTrackPoint within
 *          the configured search window. Activates the overlay when within
 *          CLIMB_ANTICIPATION_M of a segment start and keeps it visible until
 *          distRem reaches zero.
 *
 * @param lat      Current latitude.
 * @param lon      Current longitude.
 * @param simMode  True when simulation navigation is active.
 * @param simIndex Current simulation track index.
 * @param track    Loaded track points with ele and accumDist populated.
 */
void ClimbAnalyzer::updatePosition(float lat, float lon, bool simMode, int simIndex, const TrackVector& track)
{
    if (track.empty() || !hasClimbs())
        return;

    int idx = simMode
              ? simIndex
              : findClosestTrackPoint(lat, lon, track, lastIdx_);
    if (idx < 0)
        return;

    const std::vector<ClimbSegment>& segs = segments_;

    if (activeSegIdx_ >= (int)segs.size())
    {
        activeSegIdx_ = -1;
        lastIdx_      = -1;
        prevSegIdx_   = -1;
    }

    if (activeSegIdx_ < 0)
    {
        float curDist = track[idx].accumDist;
        for (int i = 0; i < (int)segs.size(); ++i)
        {
            float segStartDist = track[segs[i].startIdx].accumDist;
            float segEndDist   = track[segs[i].endIdx].accumDist;
            if (curDist <= segEndDist && segStartDist - curDist <= CLIMB_ANTICIPATION_M)
            {
                activeSegIdx_ = i;
                break;
            }
        }
    }

    if (activeSegIdx_ < 0)
        return;

    const ClimbSegment* activeSeg = &segs[activeSegIdx_];

    float curDist      = track[idx].accumDist;
    float segStartDist = track[activeSeg->startIdx].accumDist;
    float segEndDist   = track[activeSeg->endIdx].accumDist;
    float distRem;
    if (curDist < segStartDist)
        distRem = segStartDist - curDist;
    else
        distRem = (segEndDist > curDist) ? (segEndDist - curDist) : 0.0f;

    if (distRem == 0.0f)
    {
        activeSegIdx_ = -1;
        lastIdx_      = -1;
        prevSegIdx_   = -1;
        lv_subject_set_int(&subject_climb_active, 0);
        return;
    }

    float segEndEle = track[activeSeg->endIdx].ele;
    float curEle    = track[idx].ele;
    float gainRem   = (segEndEle > curEle) ? (segEndEle - curEle) : 0.0f;
    int   ahead     = (idx + 5 < (int)track.size()) ? idx + 5 : (int)track.size() - 1;
    float ddist     = track[ahead].accumDist - curDist;
    float dele      = track[ahead].ele - curEle;
    float grade     = (ddist > 1.0f) ? (dele / ddist * 100.0f) : 0.0f;
    if (grade < 0.0f)
        grade = 0.0f;

    if (activeSegIdx_ != prevSegIdx_)
    {
        prevSegIdx_ = activeSegIdx_;
        lv_subject_set_int(&subject_climb_seg,        activeSegIdx_ + 1);
        lv_subject_set_int(&subject_climb_total,      (int32_t)segs.size());
        lv_subject_set_int(&subject_climb_cat,        climbCategory(activeSeg->totalDist, activeSeg->avgGrade));
        lv_subject_set_int(&subject_climb_avg_grade,  (int32_t)(activeSeg->avgGrade * 10.0f));
        lv_subject_set_int(&subject_climb_total_dist, (int32_t)activeSeg->totalDist);
        lv_subject_set_int(&subject_climb_total_gain, (int32_t)activeSeg->totalGain);
    }
    int32_t approaching = (curDist < segStartDist) ? 1 : 0;
    lv_subject_set_int(&subject_climb_approaching, approaching);

    lastIdx_ = idx;
    lv_subject_set_int(&subject_climb_dist,  (int32_t)distRem);
    lv_subject_set_int(&subject_climb_gain,  (int32_t)gainRem);
    lv_subject_set_int(&subject_climb_grade, (int32_t)(grade * 10.0f));
    if (lv_subject_get_int(&subject_climb_active) != 1)
        lv_subject_set_int(&subject_climb_active, 1);
    if (lv_subject_get_int(&subject_climb_idx) != (int32_t)idx)
        lv_subject_set_int(&subject_climb_idx, (int32_t)idx);
}
