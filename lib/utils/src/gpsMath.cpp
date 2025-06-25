/**
 * @file gpsMath.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "gpsMath.hpp"

double midLat = 0; 
double midLon = 0; 

/**
 * @brief Function to calculate the distance in meters between two coordinates using the Haversine formula
 *
 * This function computes the great-circle distance between two points on the Earth's surface
 * given their latitude and longitude values, using the Haversine formula.
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 * @return double Distance in meters between the two points
 */
double calcDist(double lat1, double lon1, double lat2, double lon2)
{
	lat1 = lat1 * (M_PI / 180.0);
	lon1 = lon1 * (M_PI / 180.0);
	lat2 = lat2 * (M_PI / 180.0);
	lon2 = lon2 * (M_PI / 180.0);
	double dlat = lat2 - lat1;
	double dlon = lon2 - lon1;
	double a = sin(dlat / 2) * sin(dlat / 2) +
				cos(lat1) * cos(lat2) *
				sin(dlon / 2) * sin(dlon / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));
	return EARTH_RADIUS * c;
}

/**
 * @brief Function to calculate the heading (bearing) between two coordinates (latitude and longitude) using the Orthodromic (great-circle) course
 *
 * Calculates the initial bearing (forward azimuth) from the first point (lat1, lon1)
 * to the second point (lat2, lon2) on the surface of a sphere (Earth).
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 * @return double Initial heading (degrees from North, 0-360)
 */
double calcCourse(double lat1, double lon1, double lat2, double lon2)
{
	lat1 = lat1 * M_PI / 180.0;
	lat2 = lat2 * M_PI / 180.0;
	double dLon = (lon2 - lon1) * M_PI / 180.0;

	double y = sin(dLon) * cos(lat2);
	double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
	double course = atan2(y, x);

	course = course * 180.0 / M_PI;
	course = fmod((course + 360.0), 360.0);

	return course;
}

/**
 * @brief Calculates the minimum signed angular difference between two angles.
 *
 * This function computes the smallest difference between two angles (in degrees),
 * returning a value in the range [-180, 180]. Positive results indicate a
 * counterclockwise difference, and negative results indicate a clockwise difference.
 *
 * @param a Angle 1 (in degrees)
 * @param b Angle 2 (in degrees)
 * @return double Angular difference in degrees, normalized to [-180, 180]
 */
double calcAngleDiff(double a, double b)
{
  double diff = fmod((a - b + 540.0f), 360.0f) - 180.0f;
  return diff;
}

/**
 * @brief Function to calculate the midpoint between two coordinates (latitude and longitude)
 *
 * Calculates the geographic midpoint (center point) between two coordinates on the Earth's surface.
 * The result is stored in the global variables midLat and midLon.
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 */
void calcMidPoint(float lat1, float lon1, float lat2, float lon2)
{
	float dLon = (radians(lon2) - radians(lon1));
	float cosLat1 = cos(radians(lat1));
	float cosLat2 = cos(radians(lat2));
	float sinLat1 = sin(radians(lat1));
	float sinLat2 = sin(radians(lat2));
	float Bx = cosLat2 * cos(dLon);
	float By = cosLat2 * sin(dLon);

	midLat = degrees(atan2(sinLat1 + sinLat2, sqrt((cosLat1 + Bx) * (cosLat1 + Bx) + By * By)));
	midLon = degrees(radians(lon1) + atan2(By, cosLat1 + Bx));
}

/**
 * @brief Map a float value from one range to another
 *
 *
 * @param x Input value
 * @param inMin Minimum input value
 * @param inMax Maximum input value
 * @param outMin Minimum output value
 * @param outMax Maximum output value
 * @return float Mapped output value
 */
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax)
{
	return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

/**
 * @brief Converts a latitude value to a string in GGºMM'SS" format
 *
 * Converts a latitude value (in decimal degrees) to a formatted string "GGºMM'SS" N/S".
 *
 * @param lat Latitude value in decimal degrees
 * @return char* Pointer to formatted string
 */
char *latFormatString(double lat)
{
	char N_S = PSTR('N');
	double absLatitude = lat;
	uint16_t deg;
	uint8_t min;
	static char s_buf[64];
	if (lat < 0)
	{
		N_S = PSTR('S');
		absLatitude = fabs(lat);
	}
	deg = (uint16_t)absLatitude;
	absLatitude = (absLatitude - deg) * 60;
	min = (uint8_t)absLatitude;
	absLatitude = (absLatitude - min) * 60;
	sprintf(s_buf, degreeFormat, deg, min, absLatitude, N_S);
	return s_buf;
}

/**
 * @brief Converts a longitude value to a string in GGºMM'SS" format
 *
 * Converts a longitude value (in decimal degrees) to a formatted string "GGºMM'SS" E/W".
 *
 * @param lon Longitude value in decimal degrees
 * @return char* Pointer to formatted string
 */
char *lonFormatString(double lon)
{
	char E_W = PSTR('E');
	double absLongitude = lon;
	uint16_t deg;
	uint8_t min;
	static char s_buf[64];
	if (lon < 0)
	{
		E_W = PSTR('W');
		absLongitude = fabs(lon);
	}
	deg = (uint16_t)absLongitude;
	absLongitude = (absLongitude - deg) * 60;
	min = (uint8_t)absLongitude;
	absLongitude = (absLongitude - min) * 60;
	sprintf(s_buf, degreeFormat, deg, min, absLongitude, E_W);
	return s_buf;
}