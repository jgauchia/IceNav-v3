/**
 * @file gpsMath.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math and various functions
 * @version 0.2.0
 * @date 2025-04
 */

#include "gpsMath.hpp"

double midLat = 0; // Mid point between 2 Latitudes
double midLon = 0; // Mid point between 2 Longitudes

/**
 * @brief Function to calculate the distance in meters given 2 coordinates (latitude and longitude) haversine formula
 *
 * @param lat1 -> Latitude 1
 * @param lon1 -> Longitude 1
 * @param lat2 -> Latitude 2
 * @param lon2 -> Longitude 2
 * @return double -> Distance in meters
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
 * @brief Function to calculate the heading given 2 coordinates (latitude and longitude) Orthodromic Course
 *
 * @param lat1 -> Latitude 1
 * @param lon1 -> Longitude 1
 * @param lat2 -> Latitude 2
 * @param lon2 -> Longitude 2
 * @return double -> heading
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
 * @brief Function to calculate the midpoint given 2 coordinates (latitude and longitude)
 *
 * @param lat1 -> Latitude 1
 * @param lon1 -> Longitude 1
 * @param lat2 -> Latitude 2
 * @param lon2 -> Longitude 2
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
 * @brief float MAP
 *
 * @param x -> input value
 * @param inMin -> min input value
 * @param inMax -> max input value
 * @param outMin -> min output value
 * @param outMax -> max output value
 * @return float -> output value
 */
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax)
{
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

/**
 * @brief Latitude GGºMM'SS" to string conversion
 *
 * @param lat  -> Latitude
 * @return char* -> String
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
 * @brief Longitude GGºMM'SS" to string conversion
 *
 * @param lon  -> Longitude
 * @return char* -> String
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