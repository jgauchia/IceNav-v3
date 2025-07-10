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
bool lutInit = false;

/**
 * @brief Initialize lookup tables for sine and cosine
 *
 * @details Allocates memory for the tables in PSRAM if available, otherwise in RAM.
 *          Must be called once before using LUT-based trig functions.
 * @return true if initialization was successful, false if memory allocation failed
 */
bool initTrigLUT()
{
	#ifdef BOARD_HAS_PSRAM
		sinLut = (double*) heap_caps_malloc(sizeof(double) * LUT_SIZE, MALLOC_CAP_SPIRAM);
		cosLut = (double*) heap_caps_malloc(sizeof(double) * LUT_SIZE, MALLOC_CAP_SPIRAM);

		if (!sinLut || !cosLut) 
		{
			ESP_LOGE(TAGMATH, "Error: Failed to allocate memory for LUTs");
			if (sinLut) free(sinLut);
			if (cosLut) free(cosLut);
			sinLut = cosLut = NULL;
			return false;
		}
		ESP_LOGI(TAGMATH, "Allocated memory for LUTs");

		for (int i = 0; i < LUT_SIZE; ++i)
		{
			double angle = i * LUT_RES;
			sinLut[i] = sin(angle);
			cosLut[i] = cos(angle);
		}
		return true;
	#else
		return false;
    #endif
}

/**
 * @brief Function to calculate the distance in meters between two coordinates using the Haversine formula
 *
 * @details Computes the great-circle distance between two points on the Earth's surface
 * 			given their latitude and longitude values, using the Haversine formula.
 * 			Uses LUTs for trig functions if initialized.
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 * @return double Distance in meters between the two points
 */
double calcDist(double lat1, double lon1, double lat2, double lon2)
{
	lat1 = DEG2RAD(lat1);
	lon1 = DEG2RAD(lon1);
	lat2 = DEG2RAD(lat2);
	lon2 = DEG2RAD(lon2);
	double dlat = lat2 - lat1;
	double dlon = lon2 - lon1;

	double a, c;

	if (lutInit)
	{
		a = sinLUT(dlat / 2) * sinLUT(dlat / 2) +
		    cosLUT(lat1) * cosLUT(lat2) *
		    sinLUT(dlon / 2) * sinLUT(dlon / 2);
	}
	else
	{
		a = sin(dlat / 2) * sin(dlat / 2) +
			cos(lat1) * cos(lat2) *
			sin(dlon / 2) * sin(dlon / 2);
	}

	c = 2 * atan2(sqrt(a), sqrt(1 - a));

	return EARTH_RADIUS * c;
}

/**
 * @brief Function to calculate the heading (bearing) between two coordinates (latitude and longitude) using the Orthodromic (great-circle) course
 *
 * @details Calculates the initial bearing (forward azimuth) from the first point (lat1, lon1)
 * 			to the second point (lat2, lon2) on the surface of a sphere (Earth).
 *			Uses LUTs for trig functions if initialized.
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 * @return double Initial heading (degrees from North, 0-360)
 */
double calcCourse(double lat1, double lon1, double lat2, double lon2)
{

    lat1 = DEG2RAD(lat1);
    lat2 = DEG2RAD(lat2);
    double dLon = DEG2RAD(lon2 - lon1);

    double sin_dLon, cos_dLon;
    double sin_lat1, cos_lat1, sin_lat2, cos_lat2;

    if (lutInit)
    {
        sin_dLon = sinLUT(dLon);
        cos_dLon = cosLUT(dLon);
        sin_lat1 = sinLUT(lat1);
        cos_lat1 = cosLUT(lat1);
        sin_lat2 = sinLUT(lat2);
        cos_lat2 = cosLUT(lat2);
    }
    else
    {
        sin_dLon = sin(dLon);
        cos_dLon = cos(dLon);
        sin_lat1 = sin(lat1);
        cos_lat1 = cos(lat1);
        sin_lat2 = sin(lat2);
        cos_lat2 = cos(lat2);
    }

    double y = sin_dLon * cos_lat2;
    double x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dLon;
    double course = atan2(y, x) * (180.0 / M_PI);

    if (course < 0.0)
        course += 360.0;

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
	double diff = a - b;
	while (diff > 180.0) diff -= 360.0;
	while (diff < -180.0) diff += 360.0;
	return diff;
}

/**
 * @brief Function to calculate the midpoint between two coordinates (latitude and longitude)
 *
 * @details Calculates the geographic midpoint (center point) between two coordinates on the Earth's surface.
 * 			The result is stored in the global variables midLat and midLon.
 *			Uses LUTs for trig functions if initialized.
 *
 * @param lat1 Latitude of point 1 (in degrees)
 * @param lon1 Longitude of point 1 (in degrees)
 * @param lat2 Latitude of point 2 (in degrees)
 * @param lon2 Longitude of point 2 (in degrees)
 */
void calcMidPoint(float lat1, float lon1, float lat2, float lon2)
{
	double rLat1 = DEG2RAD(lat1);
	double rLon1 = DEG2RAD(lon1);
	double rLat2 = DEG2RAD(lat2);
	double rLon2 = DEG2RAD(lon2);
	double dLon  = rLon2 - rLon1;

	double sinLat1, sinLat2, cosLat1, cosLat2, cosDLon, sinDLon;

	if (lutInit)
	{
		sinLat1 = sinLUT(rLat1);
		sinLat2 = sinLUT(rLat2);
		cosLat1 = cosLUT(rLat1);
		cosLat2 = cosLUT(rLat2);
		cosDLon = cosLUT(dLon);
		sinDLon = sinLUT(dLon);
	}
	else
	{
		sinLat1 = sin(rLat1);
		sinLat2 = sin(rLat2);
		cosLat1 = cos(rLat1);
		cosLat2 = cos(rLat2);
		cosDLon = cos(dLon);
		sinDLon = sin(dLon);
	}

	double Bx = cosLat2 * cosDLon;
	double By = cosLat2 * sinDLon;
	double cosLat1_plus_Bx = cosLat1 + Bx;

	double midLatRad = atan2(sinLat1 + sinLat2, sqrt(cosLat1_plus_Bx * cosLat1_plus_Bx + By * By));
	double midLonRad = rLon1 + atan2(By, cosLat1_plus_Bx);

	midLat = RAD2DEG(midLatRad);
	midLon = RAD2DEG(midLonRad);
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
 * @details Converts a latitude value (in decimal degrees) to a formatted string "GGºMM'SS" N/S".
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
 * @details Converts a longitude value (in decimal degrees) to a formatted string "GGºMM'SS" E/W".
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