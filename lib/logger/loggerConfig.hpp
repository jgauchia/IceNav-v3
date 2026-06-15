/**
 * @file loggerConfig.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Logger activity profiles configuration
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once
#include <stdint.h>

/**
 * @brief Activity profile parameters for the GPX logger
 */
struct LoggerProfile
{
    const char* name;
    float       pauseSpeedKmh;  ///< Speed threshold below which auto-pause activates (km/h)
    uint32_t    pauseDelaySec;  ///< Seconds below threshold before actually pausing
    uint32_t    minIntervalMs;  ///< Minimum log interval at max speed (ms)
    uint32_t    maxIntervalMs;  ///< Maximum log interval at low speed (ms)
    float       minDistM;       ///< Minimum distance between points (m)
    float       maxSpeedKmh;    ///< Reference speed for interval interpolation (km/h)
};

static constexpr LoggerProfile LOGGER_PROFILES[] =
{
    { "walk", 0.5f,  5,  1000,  5000,  2.0f,  10.0f },
    { "bike", 2.0f, 10,  2000, 15000,  8.0f,  40.0f },
    { "car",  5.0f, 30,  5000, 30000, 25.0f, 130.0f },
};

static constexpr uint8_t LOGGER_PROFILE_COUNT = 3;
