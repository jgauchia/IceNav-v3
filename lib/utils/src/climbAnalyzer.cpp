/**
 * @file climbAnalyzer.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Climb profile analysis from loaded GPX track
 * @version 0.2.6
 * @date 2026-05
 */

#include "climbAnalyzer.hpp"

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
void ClimbAnalyzer::analyze(const TrackVector& trackData)
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
                    seg.startIdx  = startIdx;
                    seg.endIdx    = (int)(i - 1);
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
            seg.startIdx  = startIdx;
            seg.endIdx    = lastIdx;
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
    if (score >= 8000.0f) return 1;
    if (score >= 3500.0f) return 2;
    if (score >= 1500.0f) return 3;
    if (score >= 800.0f)  return 4;
    return 5;
}
