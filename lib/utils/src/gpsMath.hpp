/**
 * @file gpsMath.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>

#define EARTH_RADIUS 6378137             /**< Earth radius in meters */
#define METER_PER_PIXELS 156543.03       /**< Meters per pixel at zoom level 0 (latitude 0) */
#define LUT_SIZE 65536                      /**< Number of entries in the lookup tables */
#define LUT_RES (TWO_PI / (double)LUT_SIZE) /**< Angular resolution (radians per LUT step) */

// Lookup tables (allocated in PSRAM if available, else in normal RAM)
static double* sinLut = NULL;
static double* cosLut = NULL;
extern bool lutInit;

/**
 * @brief Converts degrees to radians.
 *
 * @param deg Angle in degrees.
 * @return Angle in radians.
 */
static inline __attribute__((always_inline)) double DEG2RAD(double deg)
{
    return deg * (M_PI / 180.0);
}

/**
 * @brief Converts radians to degrees.
 *
 * @param rad Angle in radians.
 * @return Angle in degrees.
 */
static inline __attribute__((always_inline)) double RAD2DEG(double rad) 
{
    return rad * (180.0 / M_PI);
}


extern double midLat;                    /**< Midpoint between two latitudes */
extern double midLon;                    /**< Midpoint between two longitudes */

static const char *degreeFormat PROGMEM = "%03d\xC2\xB0 %02d\' %.2f\" %c"; /**< Format string for degrees (DDD°MM'SS" + hemisphere) */
static const char* TAGMATH PROGMEM = "MATH";

bool initTrigLUT();

/**
 * @brief Lookup sine value using precomputed LUT
 *
 * @param rad Angle in radians
 * @return Sine of the angle
 */
static inline __attribute__((always_inline)) double sinLUT(double rad)
{
    if (!sinLut)
        return sin(rad);

    rad -= TWO_PI * floor(rad / TWO_PI);
    if (rad < 0.0) rad += TWO_PI;

    double index = rad / LUT_RES;
    int idx_low = (int)index;
    int idx_high = (idx_low + 1) % LUT_SIZE;

    double frac = index - idx_low;

    double result = sinLut[idx_low] * (1.0 - frac) + sinLut[idx_high] * frac;

    return result;
}

/**
 * @brief Lookup cosine value using precomputed LUT
 *
 * @param rad Angle in radians
 * @return Cosine of the angle
 */
static inline __attribute__((always_inline)) double cosLUT(double rad)
{
	if (!cosLut)
		return cos(rad);

	rad -= TWO_PI * floor(rad / TWO_PI);
	if (rad < 0.0) rad += TWO_PI;

	double index = rad / LUT_RES;
	int idx_low = (int)index;
	int idx_high = (idx_low + 1) % LUT_SIZE;

	double frac = index - idx_low;

	double result = cosLut[idx_low] * (1.0 - frac) + cosLut[idx_high] * frac;

	return result;
}

double calcDist(double lat1, double lon1, double lat2, double lon2);
void calcMidPoint(float lat1, float lon1, float lat2, float lon2);
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);
char *latFormatString(double lat);
char *lonFormatString(double lon);
double calcCourse(double lat1, double lon1, double lat2, double lon2);
double calcAngleDiff(double a, double b);
