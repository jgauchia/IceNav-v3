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

#define DEG2RAD(a) ((a) / (180 / M_PI))  /**< Convert degrees to radians */
#define RAD2DEG(a) ((a) * (180 / M_PI))  /**< Convert radians to degrees */

extern double midLat;                    /**< Midpoint between two latitudes */
extern double midLon;                    /**< Midpoint between two longitudes */

static const char *degreeFormat PROGMEM = "%03d\xC2\xB0 %02d\' %.2f\" %c"; /**< Format string for degrees (DDD°MM'SS" + hemisphere) */

double calcDist(double lat1, double lon1, double lat2, double lon2);
void calcMidPoint(float lat1, float lon1, float lat2, float lon2);
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);
char *latFormatString(double lat);
char *lonFormatString(double lon);
double calcCourse(double lat1, double lon1, double lat2, double lon2);
double calcAngleDiff(double a, double b);
