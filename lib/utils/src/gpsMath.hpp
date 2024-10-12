/**
 * @file gpsMath.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Math and various functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef GPSMATH_HPP
#define GPSMATH_HPP

#include <Arduino.h>
#include <stdint.h>

#define EARTH_RADIUS 6378137       // Earth Radius
#define METER_PER_PIXELS 156543.03 // Meters per pixels

#define DEG2RAD(a) ((a) / (180 / M_PI)) // Convert degrees to radians
#define RAD2DEG(a) ((a) * (180 / M_PI)) // Convert radians to degrees

extern double midLat; // Mid point between 2 Latitudes
extern double midLon; // Mid point between 2 Longitudes

static const char *degreeFormat PROGMEM = "%03d\xC2\xB0 %02d\' %.2f\" %c"; // GGºMM'SS" to string format

double calcDist(double lat1, double lon1, double lat2, double lon2);
void calcMidPoint(float lat1, float lon1, float lat2, float lon2);
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);
char *latFormatString(double lat);
char *lonFormatString(double lon);
double calcCourse(double lat1, double lon1, double lat2, double lon2);

#endif
