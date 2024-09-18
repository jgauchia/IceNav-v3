/**
 * @file renderMaps.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Render maps draw functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "renderMaps.hpp"
#include "mapsDrawFunc.h"

extern const int SD_CS;

MapTile oldMapTile = {"", 0, 0, 0};     // Old Map tile coordinates and zoom
MapTile currentMapTile = {"", 0, 0, 0}; // Current Map tile coordinates and zoom
MapTile roundMapTile = {"", 0, 0, 0};   // Boundaries Map tiles
ScreenCoord navArrowPosition = {0,0};  // Map Arrow position

bool isMapFound = false;

tileBounds totalBounds = { 90.0, -90.0, 180.0, -180.0}; 



/**
 * @brief Tile size for position calculation
 *
 */
uint16_t tileSize = 256;

TFT_eSprite sprArrow = TFT_eSprite(&tft);      // Sprite for Navigation Arrow in map tile
TFT_eSprite mapTempSprite = TFT_eSprite(&tft); // Temporary Map Sprite 9x9 tiles 768x768 pixels
TFT_eSprite mapSprite = TFT_eSprite(&tft);     // Screen Map Sprite (Viewed in LVGL Tile)

/**
 * @brief Get TileX for OpenStreetMap files
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
 * @brief Get Longitude from OpenStreetMap files
 *
 * @param tileX -> tile X
 * @param zoom  -> zoom
 * @return longitude
 */
double tilex2lon(uint32_t tileX, uint8_t zoom)
{
  return tileX / pow(2.0, zoom) * 360.0 - 180.0;
}

/**
 * @brief Get Latitude from OpenStreetMap files
 *
 * @param tileX -> tile Y
 * @param zoom  -> zoom
 * @return latitude
 */
double tiley2lat(uint32_t tileY, uint8_t zoom)
{
  double n = M_PI - 2.0 * M_PI * tileY / pow(2.0, zoom);
  return 180.0 / M_PI * atan(sinh(n));
}

/**
 * @brief Get min and max longitude and latitude from tile
 *
 * @param tileX -> tile X
 * @param tileY -> tile Y
 * @param zoom  -> zoom
 * @return tileBounds -> min and max longitude and latitude
 */
tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom)
{
  tileBounds bounds;
  bounds.lon_min = tilex2lon(tileX, zoom);
  bounds.lat_min = tiley2lat(tileY + 1, zoom);  
  bounds.lon_max = tilex2lon(tileX + 1, zoom);  
  bounds.lat_max = tiley2lat(tileY, zoom);      
  return bounds;
}

/**
 * @brief Get the map tile structure from GPS Coordinates
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> zoom level
 * @param offsetX -> Tile Offset X
 * @param offsetY -> Tile Offset Y
 * @return MapTile -> Map Tile structure
 */
MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY)
{
  static char tileFile[40] = "";
  uint32_t x = lon2tilex(lon, zoomLevel) + offsetX;
  uint32_t y = lat2tiley(lat, zoomLevel) + offsetY;

  sprintf(tileFile, mapFolder, zoomLevel, x, y);
  MapTile data;
  data.file = tileFile;
  data.tilex = x;
  data.tiley = y;
  data.zoom = zoomLevel;
  return data;
}

/**
 * @brief Generate render map
 *
 */
void generateRenderMap()
{
  currentMapTile = getMapTile(getLon(), getLat(), zoom, 0, 0);
  bool foundRoundMap = false;

  // Detects if tile changes from actual GPS position
  if (strcmp(currentMapTile.file, oldMapTile.file) != 0 || currentMapTile.zoom != oldMapTile.zoom || 
             currentMapTile.tilex != oldMapTile.tilex || currentMapTile.tiley != oldMapTile.tiley)
  {
    acquireSdSPI();

    // deleteMapScrSprites();
    // createMapScrSprites();

    isMapFound  = mapTempSprite.drawPngFile(SD, currentMapTile.file, tileSize, tileSize);

    if (!isMapFound)
    {
      log_v("No Map Found!");
      isMapFound = false;
      oldMapTile.file = currentMapTile.file;
      oldMapTile.zoom = currentMapTile.zoom;
      oldMapTile.tilex = currentMapTile.tilex;
      oldMapTile.tiley = currentMapTile.tiley;
      mapTempSprite.fillScreen(TFT_BLACK);
      showNoMap(mapTempSprite);
    }
    else
    {
      log_v("Map Found!");
      oldMapTile.file = currentMapTile.file;

      static const uint8_t centerX = 0;
      static const uint8_t centerY = 0;
      int8_t startX = centerX - 1;
      int8_t startY = centerY - 1;

      for (int y = startY; y <= startY + 2; y++)
      {
        for (int x = startX; x <= startX + 2; x++)
        {
          if (x == centerX && y == centerY)
          {
            // Skip Center Tile
            continue;
          }
          roundMapTile = getMapTile(getLon(), getLat(), zoom, x, y);

          tileBounds currentBounds = getTileBounds(roundMapTile.tilex, roundMapTile.tiley, zoom);
                    
          if (currentBounds.lat_min < totalBounds.lat_min) totalBounds.lat_min = currentBounds.lat_min;
          if (currentBounds.lat_max > totalBounds.lat_max) totalBounds.lat_max = currentBounds.lat_max;
          if (currentBounds.lon_min < totalBounds.lon_min) totalBounds.lon_min = currentBounds.lon_min;
          if (currentBounds.lon_max > totalBounds.lon_max) totalBounds.lon_max = currentBounds.lon_max;

          foundRoundMap = mapTempSprite.drawPngFile(SD, roundMapTile.file, (x - startX) * tileSize, (y - startY) * tileSize);
          if (!foundRoundMap)
          {
            mapTempSprite.fillRect((x - startX) * tileSize, (y - startY) * tileSize, tileSize, tileSize, TFT_BLACK);
            mapTempSprite.drawPngFile(noMapFile, ((x - startX) * tileSize) + (tileSize / 2) - 50 , ((y - startY) * tileSize) + (tileSize / 2) - 50);
          }
        }
      }

      log_i("Total Bounds: Lat Min: %f, Lat Max: %f, Lon Min: %f, Lon Max: %f",
            totalBounds.lat_min, totalBounds.lat_max, totalBounds.lon_min, totalBounds.lon_max);

      oldMapTile.zoom = currentMapTile.zoom;
      oldMapTile.tilex = currentMapTile.tilex;
      oldMapTile.tiley = currentMapTile.tiley;

      redrawMap = true;
    }

    releaseSdSPI();

    log_v("TILE: %s", oldMapTile.file);
  }
}
