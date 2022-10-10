/**
 * @file math.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Math routines
 * @version 0.1
 * @date 2022-10-09
 */

/**
 * @brief float MAP
 * 
 * @param x -> input value
 * @param in_min -> min input value
 * @param in_max -> max input value
 * @param out_min -> min output value 
 * @param out_max -> max output value
 * @return float -> output value
 */
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief RAD to DEG conversion
 * 
 * @param rad -> radians
 * @return double -> degrees
 */
double RADtoDEG(double rad)
{
  return rad * 180 / PI;
}

/**
 * @brief DEG to RAD conversion
 * 
 * @param deg -> degrees
 * @return double -> radians
 */
double DEGtoRAD(double deg)
{
  return deg * PI / 180;
}

/**
 * @brief Get TileY for OpenStreeMap files
 * 
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return int -> X value (folder)
 */
int lon2tilex(double f_lon, int zoom)
{
   return (int)(floor((f_lon + 180.0) / 360.0 * pow(2.0, zoom))); 
}

/**
 * @brief Get TileY for OpenStreetMap files
 * 
 * @param f_lat -> latitude
 * @param zoom  -> zoom
 * @return int -> Y value (file)
 */
int lat2tiley(double f_lat, int zoom)
{
  return (int)(floor((1.0 - log( tan(f_lat * M_PI/180.0) + 1.0 / cos(f_lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, zoom))); 
}

/**
 * @brief Get pixel X position from OpenStreetMap 
 * 
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return int -> X position
 */
int lon2posx(float f_lon, int zoom)
{
   return ((int)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom))*256)) % 256); 
}

/**
 * @brief Get pixel Y position from OpenStreetMap
 * 
 * @param f_lat -> latitude
 * @param zoom -> zoom
 * @return int -> Y position
 */
int lat2posy(float f_lat, int zoom)
{
   return ((int)(((1.0 - log( tan(f_lat * M_PI/180.0) + 1.0 / cos(f_lat * M_PI/180.0)) / M_PI) / 2.0 * (pow(2.0, zoom))*256)) % 256); 
}

/**
 * @brief Latitude GGºMM'SS" to string conversion and draw on screen in X,Y position and font type
 * 
 * @param x -> X Position
 * @param y -> Y Position
 * @param font -> Font type
 * @param lat  -> Latitude
 */
void Latitude_formatString(int x, int y, int font,  double lat)
{
  char N_S = 'N';
  double absLatitude = lat;
  uint16_t deg;
  uint8_t min;
  if (lat < 0)
  {
    N_S = 'S';
    absLatitude = fabs(lat);
  }
  deg = (uint16_t) absLatitude;
  absLatitude = (absLatitude - deg) * 60;
  min = (uint8_t) absLatitude;
  absLatitude = (absLatitude - min) * 60;
  tft.setTextFont(font);
  tft.setCursor(x, y, font);
  sprintf(s_buf, "%03d ", deg);
  tft.print(s_buf);
  tft.print("`");
  sprintf(s_buf, "%02d\' %.2f\" %c", min, absLatitude, N_S);
  tft.print(s_buf);
}

/**
 * @brief Longitude GGºMM'SS" to string conversion and draw on screen in X,Y position and font type
 * 
 * @param x -> X Position
 * @param y -> Y Position
 * @param font -> Font type
 * @param lat  -> Latitude
 */
void Longitude_formatString(int x, int y, int font,  double lon)
{
  char E_W = 'E';
  double absLongitude = lon;
  uint16_t deg;
  uint8_t min;
  if (lon < 0)
  {
    E_W = 'W';
    absLongitude = fabs(lon);
  }
  deg = (uint16_t) absLongitude;
  absLongitude = (absLongitude - deg) * 60;
  min = (uint8_t) absLongitude;
  absLongitude = (absLongitude - min) * 60;
  tft.setTextFont(font);
  tft.setCursor(x, y, font);
  sprintf(s_buf, "%03d ", deg);
  tft.print(s_buf);
  tft.print("`");
  sprintf(s_buf, "%02d\' %.2f\" %c", min, absLongitude, E_W);
  tft.print(s_buf);
}
