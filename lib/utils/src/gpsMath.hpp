/**
 * @file gpsMath.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <stdint.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef TWO_PI
#define TWO_PI (2.0f * M_PI)
#endif

#define EARTH_RADIUS 6378137.0f          /**< Earth radius in meters */
#define METER_PER_PIXELS 156543.03f      /**< Meters per pixel at zoom level 0 (latitude 0) */
#define LUT_SIZE 65536                      /**< Number of entries in the lookup tables */
static const float LUT_RES = (TWO_PI / (float)LUT_SIZE); /**< Angular resolution (radians per LUT step) */

// Lookup tables (allocated in PSRAM if available, else in normal RAM)
static float* sinLut = NULL;
static float* cosLut = NULL;
extern bool lutInit;

/**
 * @brief Converts degrees to radians.
 *
 * @param deg Angle in degrees.
 * @return Angle in radians.
 */
static inline __attribute__((always_inline)) float DEG2RAD(float deg)
{
    return deg * (M_PI / 180.0f);
}

/**
 * @brief Converts radians to degrees.
 *
 * @param rad Angle in radians.
 * @return Angle in degrees.
 */
static inline __attribute__((always_inline)) float RAD2DEG(float rad)
{
    return rad * (180.0f / M_PI);
}

static const char *degreeFormat = "%03d\xC2\xB0 %02d\' %.2f\" %c"; /**< Format string for degrees (DDD°MM'SS" + hemisphere) */
static const char* TAGMATH = "MATH";

bool initTrigLUT();

/**
 * @brief Lookup sine value using precomputed LUT
 *
 * @param rad Angle in radians
 * @return Sine of the angle
 */
static inline __attribute__((always_inline)) float sinLUT(float rad)
{
    if (!sinLut)
        return sinf(rad);

    rad -= TWO_PI * floorf(rad / TWO_PI);
    if (rad < 0.0f) rad += TWO_PI;

    float index = rad / LUT_RES;
    int idx_low = (int)index;
    int idx_high = (idx_low + 1) % LUT_SIZE;

    float frac = index - idx_low;

    float result = sinLut[idx_low] * (1.0f - frac) + sinLut[idx_high] * frac;

    return result;
}

/**
 * @brief Lookup cosine value using precomputed LUT
 *
 * @param rad Angle in radians
 * @return Cosine of the angle
 */
static inline __attribute__((always_inline)) float cosLUT(float rad)
{
	if (!cosLut)
		return cosf(rad);

	// Normalize angle to [0, TWO_PI)
	rad -= TWO_PI * floorf(rad / TWO_PI);
	if (rad < 0.0f) 
        rad += TWO_PI;

	float index = rad / (float)LUT_RES;
	int idx_low = (int)index;
	int idx_high = (idx_low + 1) % LUT_SIZE;

	float frac = index - idx_low;

	float result = (float)cosLut[idx_low] * (1.0f - frac) + (float)cosLut[idx_high] * frac;

	return result;
}


float calcDist(float lat1, float lon1, float lat2, float lon2);
float calcDistSq(float lat1, float lon1, float lat2, float lon2);
float calcCourse(float lat1, float lon1, float lat2, float lon2);
float calcAngleDiff(float a, float b);
char *latFormatString(float lat);
char *lonFormatString(float lon);
