/**
 * @file climbAnalyzer.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Climb profile analysis from loaded GPX track
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include <vector>
#include "globalGpxDef.h"

static constexpr float CLIMB_MIN_DIST_M    = 500.0f;
static constexpr float CLIMB_MIN_GRADE     = 3.0f;
static constexpr float CLIMB_ANTICIPATION_M = 1000.0f;

static constexpr int TRI_ROWS   = 5;
static constexpr int TRI_GAP    = 2;
static constexpr int TRI_MARGIN = TRI_ROWS + TRI_GAP + 1;

extern const int triMask[TRI_ROWS];

struct ClimbSegment
{
    int   startIdx;
    int   endIdx;
    float totalDist;
    float totalGain;
    float avgGrade;
};

class ClimbAnalyzer
{
public:
    void analyze(const TrackVector& trackData, int startOffset = 0);
    bool hasClimbs() const { return !segments_.empty(); }
    const std::vector<ClimbSegment>& segments() const { return segments_; }
    void clear();

    void updatePosition(float lat, float lon, bool simMode, int simIndex, const TrackVector& track);

private:
    std::vector<ClimbSegment> segments_;
    int activeSegIdx_ = -1;
    int lastIdx_      = -1;
    int prevSegIdx_   = -1;
};

int      climbCategory(float totalDist, float avgGrade);
uint32_t climbSegmentColor(float avgGrade);
int      calcYTop(float ele, float minEle, float eleRange, int H);
