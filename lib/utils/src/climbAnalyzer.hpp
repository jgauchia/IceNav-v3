/**
 * @file climbAnalyzer.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Climb profile analysis from loaded GPX track
 * @version 0.2.6
 * @date 2026-05
 */

#pragma once

#include <vector>
#include "globalGpxDef.h"

/**
 * @brief Minimum sustained distance (m) to qualify as a significant climb
 */
static constexpr float CLIMB_MIN_DIST_M = 500.0f;

/**
 * @brief Minimum average grade (%) to qualify as a climb
 */
static constexpr float CLIMB_MIN_GRADE = 3.0f;

/**
 * @brief Look-ahead distance (m) before a climb start to activate the overlay
 */
static constexpr float CLIMB_ANTICIPATION_M = 1000.0f;

/**
 * @brief Represents a detected climb segment within a GPX track
 */
struct ClimbSegment
{
    int   startIdx;     /**< First trackData index of the climb */
    int   endIdx;       /**< Last  trackData index of the climb */
    float totalDist;    /**< Horizontal distance of the climb (meters) */
    float totalGain;    /**< Total elevation gain of the climb (meters) */
    float   avgGrade;     /**< Average grade of the climb (percent) */
};

/**
 * @class ClimbAnalyzer
 * @brief Pre-computes climb segments from a loaded TrackVector.
 *
 * @details Call analyze() once after loadTrack(). Then iterate
 *          segments() on each GPS position update to retrieve
 *          live climb progress without touching the SD card.
 */
class ClimbAnalyzer
{
public:
    void analyze(const TrackVector& trackData);
    bool hasClimbs() const { return !segments_.empty(); }
    const std::vector<ClimbSegment>& segments() const { return segments_; }
    void clear() { segments_.clear(); }

private:
    std::vector<ClimbSegment> segments_;
};

int climbCategory(float totalDist, float avgGrade);
