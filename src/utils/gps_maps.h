/**
 * @file gps_maps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS Maps functions
 * @version 0.1.4
 * @date 2023-05-23
 */

/**
 * @brief Structure to store Map tile filename, actual tileX, tileY and zoom level
 *
 */
struct MapTile
{
  char *file;
  uint32_t tilex;
  uint32_t tiley;
  uint8_t zoom;
};

/**
 * @brief Get TileY for OpenStreeMap files
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return X value (folder)
 */
uint32_t lon2tilex(double f_lon, uint8_t zoom)
{
  return (uint32_t)(floor((f_lon + 180.0) / 360.0 * pow(2.0, zoom)));
}

/**
 * @brief Get TileY for OpenStreetMap files
 *
 * @param f_lat -> latitude
 * @param zoom  -> zoom
 * @return Y value (file)
 */
uint32_t lat2tiley(double f_lat, uint8_t zoom)
{
  return (uint32_t)(floor((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, zoom)));
}

/**
 * @brief Get pixel X position from OpenStreetMap
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return X position
 */
uint16_t lon2posx(float f_lon, uint8_t zoom)
{
  return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * 256)) % 256);
}

/**
 * @brief Get pixel Y position from OpenStreetMap
 *
 * @param f_lat -> latitude
 * @param zoom -> zoom
 * @return Y position
 */
uint16_t lat2posy(float f_lat, uint8_t zoom)
{
  return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * 256)) % 256);
}

/**
 * @brief Get the map tile structure from GPS Coordinates
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoom_level -> zoom level
 * @param off_x -> Tile Offset X
 * @param off_y -> Tile Offset Y
 * @return MapTile -> Map Tile structure
 */
MapTile get_map_tile(double lon, double lat, uint8_t zoom_level, int16_t off_x, int16_t off_y)
{
  static char s_file[40] = "";
  uint32_t x = lon2tilex(lon, zoom_level) + off_x;
  uint32_t y = lat2tiley(lat, zoom_level) + off_y;

  sprintf(s_file, PSTR("/MAP/%d/%d/%d.png"), zoom_level, x, y);
  MapTile data;
  data.file = s_file;
  data.tilex = x;
  data.tiley = y;
  data.zoom = zoom_level;
  return data;
}
