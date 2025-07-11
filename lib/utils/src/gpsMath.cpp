/**
 * @file gpsMath.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "gpsMath.hpp"

float midLat = 0; 
float midLon = 0; 
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
    sinLut = (float*) heap_caps_malloc(sizeof(float) * LUT_SIZE, MALLOC_CAP_SPIRAM);
    cosLut = (float*) heap_caps_malloc(sizeof(float) * LUT_SIZE, MALLOC_CAP_SPIRAM);

    if (!sinLut || !cosLut) 
    {
        ESP_LOGE(TAGMATH, "Error: Failed to allocate memory for float LUTs");
        if (sinLut) free(sinLut);
        if (cosLut) free(cosLut);
        sinLut = cosLut = NULL;
        return false;
    }
    ESP_LOGI(TAGMATH, "Allocated memory for float LUTs");

    for (int i = 0; i < LUT_SIZE; ++i)
    {
        float angle = i * LUT_RES;
        sinLut[i] = sinf(angle);
        cosLut[i] = cosf(angle);
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
 * @return Distance in meters between the two points
 */
float calcDist(float lat1, float lon1, float lat2, float lon2)
{
	lat1 = DEG2RAD(lat1);
	lon1 = DEG2RAD(lon1);
	lat2 = DEG2RAD(lat2);
	lon2 = DEG2RAD(lon2);
 	float dlat = lat2 - lat1;
    float dlon = lon2 - lon1;

    float a, c;

    if (lutInit)
    {
        a = sinLUT(dlat * 0.5f) * sinLUT(dlat * 0.5f) +
            cosLUT(lat1) * cosLUT(lat2) *
            sinLUT(dlon * 0.5f) * sinLUT(dlon * 0.5f);
    }
    else
    {
        a = sinf(dlat * 0.5f) * sinf(dlat * 0.5f) +
            cosf(lat1) * cosf(lat2) *
            sinf(dlon * 0.5f) * sinf(dlon * 0.5f);
    }

    c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));

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
 * @return Initial heading (degrees from North, 0-360)
 */
float calcCourse(float lat1, float lon1, float lat2, float lon2)
{
    lat1 = DEG2RAD(lat1);
    lat2 = DEG2RAD(lat2);
    float dLon = DEG2RAD(lon2 - lon1);

    float sin_dLon, cos_dLon;
    float sin_lat1, cos_lat1, sin_lat2, cos_lat2;

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
        sin_dLon = sinf(dLon);
        cos_dLon = cosf(dLon);
        sin_lat1 = sinf(lat1);
        cos_lat1 = cosf(lat1);
        sin_lat2 = sinf(lat2);
        cos_lat2 = cosf(lat2);
    }

    float y = sin_dLon * cos_lat2;
    float x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dLon;
    float course = atan2f(y, x) * (180.0f / M_PI);

    if (course < 0.0f)
        course += 360.0f;

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
 * @return Angular difference in degrees, normalized to [-180, 180]
 */
float calcAngleDiff(float a, float b)
{
    float diff = a - b;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
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
	float rLat1 = DEG2RAD(lat1);
	float rLon1 = DEG2RAD(lon1);
	float rLat2 = DEG2RAD(lat2);
	float rLon2 = DEG2RAD(lon2);
	float dLon  = rLon2 - rLon1;

	float sinLat1, sinLat2, cosLat1, cosLat2, cosDLon, sinDLon;;

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
		sinLat1 = sinf(rLat1);
		sinLat2 = sinf(rLat2);
		cosLat1 = cosf(rLat1);
		cosLat2 = cosf(rLat2);
		cosDLon = cosf(dLon);
		sinDLon = sinf(dLon);
	}

	float Bx = cosLat2 * cosDLon;
	float By = cosLat2 * sinDLon;
	float cosLat1_plus_Bx = cosLat1 + Bx;

	float midLatRad = atan2f(sinLat1 + sinLat2, sqrtf(cosLat1_plus_Bx * cosLat1_plus_Bx + By * By));
	float midLonRad = rLon1 + atan2f(By, cosLat1_plus_Bx);

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
 * @brief Format latitude in degrees°minutes'seconds"N/S string.
 *
 * @details Converts a float latitude into a formatted string using degrees, minutes,
 *          and seconds notation (e.g., 41°23'45.12"N or S).
 *          Uses a static buffer and assumes `degreeFormat` is a valid format string.
 *
 * @param lat Latitude in decimal degrees.
 * @return Pointer to static buffer containing formatted string.
 */
char *latFormatString(float lat)
{
    char N_S = 'N';
    float absLatitude = lat;
    uint16_t deg;
    uint8_t min;
    static char s_buf[64];

    if (lat < 0.0f)
    {
        N_S = 'S';
        absLatitude = fabsf(lat);
    }

    deg = static_cast<uint16_t>(absLatitude);
    absLatitude = (absLatitude - deg) * 60.0f;
    min = static_cast<uint8_t>(absLatitude);
    absLatitude = (absLatitude - min) * 60.0f;

    sprintf(s_buf, degreeFormat, deg, min, absLatitude, N_S);
    return s_buf;
}

/**
 * @brief Format longitude in degrees°minutes'seconds"E/W string.
 *
 * @details Converts a float longitude into a formatted string using degrees, minutes,
 *          and seconds notation (e.g., 2°10'30.45"E or W).
 *          Uses a static buffer and assumes `degreeFormat` is a valid format string.
 *
 * @param lon Longitude in decimal degrees.
 * @return Pointer to static buffer containing formatted string.
 */
char *lonFormatString(float lon)
{
    char E_W = 'E';
    float absLongitude = lon;
    uint16_t deg;
    uint8_t min;
    static char s_buf[64];

    if (lon < 0.0f)
    {
        E_W = 'W';
        absLongitude = fabsf(lon);
    }

    deg = static_cast<uint16_t>(absLongitude);
    absLongitude = (absLongitude - deg) * 60.0f;
    min = static_cast<uint8_t>(absLongitude);
    absLongitude = (absLongitude - min) * 60.0f;

    sprintf(s_buf, degreeFormat, deg, min, absLongitude, E_W);
    return s_buf;
}
