/**
 * @file gpsMath.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Math and various functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "gpsMath.hpp"

double midLat = 0; // Mid point between 2 Latitudes
double midLon = 0; // Mid point between 2 Longitudes

/**
 * @brief Function to calculate the distance in meters given 2 coordinates (latitude and longitude)
 *
 * @param lat1 -> Latitude 1
 * @param lon1 -> Longitude 1
 * @param lat2 -> Latitude 2
 * @param lon2 -> Longitude 2
 * @return float -> Distance in meters
 */
float calcDist(float lat1, float lon1, float lat2, float lon2)
{
  float f_x = 69.1 * (lat2 - lat1);
  float f_y = 69.1 * (lon2 - lon1) * cos(lat1 / 57.3);
  return (float)sqrt((float)(f_x * f_x) + (float)(f_y * f_y)) * 1609.344;
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