/**
 * @file gpsMath.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.9
 * @date 2026-06
 */

#include "gpsMath.hpp"
#include <cstdio>
#include "esp_log.h"
#include "esp_heap_caps.h"

bool lutInit = false;
float* sinLut = NULL;
float* cosLut = NULL;

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
            if (sinLut) 
                free(sinLut);
            if (cosLut) 
                free(cosLut);
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
	float lat1_rad = DEG2RAD(lat1);
	float lon1_rad = DEG2RAD(lon1);
	float lat2_rad = DEG2RAD(lat2);
	float lon2_rad = DEG2RAD(lon2);
 	float dlat = lat2_rad - lat1_rad;
    float dlon = lon2_rad - lon1_rad;

    float a;
    float c;

    if (lutInit)
    {
        a = sinLUT(dlat * 0.5f) * sinLUT(dlat * 0.5f) +
            cosLUT(lat1_rad) * cosLUT(lat2_rad) *
            sinLUT(dlon * 0.5f) * sinLUT(dlon * 0.5f);
    }
    else
    {
        a = sinf(dlat * 0.5f) * sinf(dlat * 0.5f) +
            cosf(lat1_rad) * cosf(lat2_rad) *
            sinf(dlon * 0.5f) * sinf(dlon * 0.5f);
    }

    c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return EARTH_RADIUS * c;
}

/**
 * @brief Fast squared distance calculation using Equirectangular approximation.
 * 
 * @details Valid for short distances. Returns distance squared in meters^2 to avoid costly sqrtf.
 *          Useful for comparing distances in loops. Expects coordinates in RADIANS.
 * 
 * @param lat1_rad Latitude 1 (radians)
 * @param lon1_rad Longitude 1 (radians)
 * @param lat2_rad Latitude 2 (radians)
 * @param lon2_rad Longitude 2 (radians)
 * @return Squared distance in meters^2
 */
float calcDistSq(float lat1_rad, float lon1_rad, float lat2_rad, float lon2_rad)
{
    float x = (lon2_rad - lon1_rad) * cosf((lat1_rad + lat2_rad) / 2.0f);
    float y = lat2_rad - lat1_rad;

    return (x * x + y * y);
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

    float sin_dLon;
    float cos_dLon;
    float sin_lat1;
    float cos_lat1;
    float sin_lat2;
    float cos_lat2;

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
    while (diff > 180.0f)
        diff -= 360.0f;
    while (diff < -180.0f)
        diff += 360.0f;
    return diff;
}

/**
 * @brief Formats a coordinate (latitude or longitude) into a degrees°minutes'seconds"H string.
 *
 * @details Shared implementation used by latFormatString() and lonFormatString().
 *          Converts a decimal degree value to DMS notation with hemisphere indicator.
 *
 * @param value     Coordinate value in decimal degrees.
 * @param posHemi   Hemisphere character for positive values (e.g. 'N' or 'E').
 * @param negHemi   Hemisphere character for negative values (e.g. 'S' or 'W').
 * @param buf       Output buffer to write the formatted string.
 * @param bufSize   Size of the output buffer.
 */
static void formatCoordinate(float value, char posHemi, char negHemi, char *buf, size_t bufSize)
{
    char hemi = (value < 0.0f) ? negHemi : posHemi;
    float absVal = fabsf(value);
    uint16_t deg = static_cast<uint16_t>(absVal);
    absVal = (absVal - deg) * 60.0f;
    uint8_t min = static_cast<uint8_t>(absVal);
    absVal = (absVal - min) * 60.0f;
    snprintf(buf, bufSize, degreeFormat, deg, min, absVal, hemi);
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
    static char s_buf[64];
    formatCoordinate(lat, 'N', 'S', s_buf, sizeof(s_buf));
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
    static char s_buf[64];
    formatCoordinate(lon, 'E', 'W', s_buf, sizeof(s_buf));
    return s_buf;
}
