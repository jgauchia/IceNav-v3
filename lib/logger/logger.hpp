/**
 * @file logger.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Data Logger — class declaration
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Snapshot of GPS data passed to GpxLogger::update().
 *
 * @details Decouples the logger from NeoGPS / gps.hpp headers.
 *          Filled by gpsTask which already holds gpsMutex and has gps.hpp in scope.
 */
struct LoggerGpsFix
{
    float   lat;
    float   lon;
    int16_t alt;
    float   speedKmh;
    bool    valid;      ///< fix.valid.location && isGpsFixed
    bool    hasTime;    ///< fix.valid.time && fix.valid.date
    uint16_t year;      ///< 4-digit (2000 + fix.dateTime.year)
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
};

/**
 * @brief Logger state machine states
 */
enum class LoggerState : uint8_t
{
    IDLE       = 0,
    RECORDING  = 1,
    AUTO_PAUSE = 2,
};

/**
 * @brief Statistics collected during and after a recording session
 */
struct LoggerStats
{
    float    totalDistM;     ///< Total track distance (metres)
    float    maxSpeedKmh;    ///< Maximum speed recorded (km/h)
    float    avgSpeedKmh;    ///< Average moving speed (km/h)
    uint32_t movingTimeSec;  ///< Time in motion (seconds)
    uint32_t totalTimeSec;   ///< Wall-clock recording time (seconds)
    int32_t  gainPos;        ///< Cumulative positive elevation gain (m)
    int32_t  gainNeg;        ///< Cumulative negative elevation loss (m)
    uint32_t numPoints;      ///< Number of trackpoints written
    char     filename[64];   ///< Full path of the GPX file
};

/**
 * @brief GPX Data Logger
 *
 * @details Thread-safe logger that hooks into gpsTask to write GPX 1.1 tracks.
 *          update() is called from gpsTask (holds gpsMutex) and uses mutex
 *          with timeout 0 to avoid blocking. start()/stop() are called from
 *          the UI thread (holds lvgl_mutex) and use mutex with 100 ms timeout.
 */
class GpxLogger
{
public:
    GpxLogger() = default;

    void init();                            ///< Create /sdcard/TRK/, initialise mutex
    void update(const LoggerGpsFix& fix);   ///< Hook in gpsTask — called inside gpsMutex
    void start();               ///< Begin a new recording (called from UI)
    void stop();                ///< End current recording (called from UI)
    void setProfile(uint8_t idx);

    LoggerState        state()           const { return loggerState; }
    const LoggerStats& stats()           const { return trackStats; }
    uint8_t            profileIndex()    const { return profileIdx; }
    uint32_t           movingElapsedMs() const { return movingMs; }
    float              currentGrade()    const { return lastGrade; }

private:
    void     writeTrkpt(float lat, float lon, int16_t alt, float speedKmh, const LoggerGpsFix& fix);
    uint32_t adaptiveInterval(float speedKmh) const;

    volatile LoggerState loggerState         = LoggerState::IDLE;
    FILE*                file          = nullptr;
    uint8_t              profileIdx    = 0;
    bool                 segOpen       = false;

    uint32_t startMs        = 0;
    uint32_t lastLogMs      = 0;
    uint32_t lastUpdateMs   = 0;
    uint32_t pauseStartMs   = 0;
    uint32_t movingMs       = 0;
    uint32_t flushCnt       = 0;

    float lastLat      = 0.0f;
    float lastLon      = 0.0f;
    float lastAlt      = 0.0f;
    float lastGrade    = 0.0f;
    float gradeAccDist = 0.0f;
    float gradeAccAlt  = 0.0f;
    bool  hasLast      = false;

    float    speedAccum = 0.0f;
    uint32_t speedSamps = 0;

    LoggerStats       trackStats = {};
    SemaphoreHandle_t mutex = nullptr;
};

extern GpxLogger gpxLogger;
