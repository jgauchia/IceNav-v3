/**
 * @file render_maps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS Maps functions
 * @version 0.1.6
 * @date 2023-06-14
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
 * @brief Old Map tile coordinates and zoom
 *
 */
MapTile OldMapTile = {"", 0, 0, 0};

/**
 * @brief Map tile coordinates and zoom
 *
 */
MapTile CurrentMapTile;
MapTile RoundMapTile;

/**
 * @brief Tile size for position calculation
 *
 */
uint16_t tileSize = 256;

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
  return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
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
  return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Convert GPS Coordinates to screen position (with offsets)
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoom_level -> Zoom level
 * @return ScreenCoord -> Screen position
 */
ScreenCoord coord_to_scr_pos(double lon, double lat, uint8_t zoom_level)
{
  ScreenCoord data;
  data.posx = lon2posx(lon, zoom_level);
  data.posy = lat2posy(lat, zoom_level);
  return data;
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

/**
 * @brief Generate render map
 *
 */
void generate_render_map()
{
  CurrentMapTile = get_map_tile(getLon(), getLat(), zoom, 0, 0);

  if (strcmp(CurrentMapTile.file, OldMapTile.file) != 0 || CurrentMapTile.zoom != OldMapTile.zoom ||
      CurrentMapTile.tilex != OldMapTile.tilex || CurrentMapTile.tiley != OldMapTile.tiley)
  {
    is_map_draw = false;
    map_found = false;
  }

  if (!is_map_draw)
  {
    OldMapTile.zoom = CurrentMapTile.zoom;
    OldMapTile.tilex = CurrentMapTile.tilex;
    OldMapTile.tiley = CurrentMapTile.tiley;
    OldMapTile.file = CurrentMapTile.file;

    log_v("TILE: %s", CurrentMapTile.file);
    log_v("ZOOM: %d", zoom);

    // Center Tile
    map_found = map_spr.drawPngFile(SD, CurrentMapTile.file, 256, 256);

    uint8_t centerX = 0;
    uint8_t centerY = 0;
    int8_t startX = centerX - 1;
    int8_t startY = centerY - 1;
    bool tileFound = false;

    if (map_found)
    {
      for (int y = startY; y <= startY + 2; y++)
      {
        for (int x = startX; x <= startX + 2; x++)
        {
          if (x == centerX && y == centerY)
          {
            // Skip Center Tile
            continue;
          }
          RoundMapTile = get_map_tile(getLon(), getLat(), zoom, x, y);
          tileFound = map_spr.drawPngFile(SD, RoundMapTile.file, (x - startX) * tileSize, (y - startY) * tileSize);
          if (!tileFound)
            map_spr.fillRect((x - startX) * tileSize, (y - startY) * tileSize, tileSize, tileSize, LVGL_BKG);
        }
      }
    }

    is_map_draw = true;
  }

  if (map_found)
  {
    NavArrow_position = coord_to_scr_pos(getLon(), getLat(), zoom);
    map_spr.setPivot(tileSize + NavArrow_position.posx, tileSize + NavArrow_position.posy);
    map_rot.pushSprite(0, 27);

#ifdef ENABLE_COMPASS
    heading = get_heading();
    if (map_rotation)
      map_heading = get_heading();
    else
      map_heading = GPS.course.deg();
    map_spr.pushRotated(&map_rot, 360 - map_heading, TFT_TRANSPARENT);
#else
    map_heading = GPS.course.deg();
    map_spr.pushRotated(&map_rot, 360 - map_heading, TFT_TRANSPARENT);
    // map_spr.pushRotated(&map_rot, 0, TFT_TRANSPARENT);
#endif
    draw_map_widgets();

    sprArrow.pushRotated(&map_rot, 0, TFT_BLACK);
  }
}