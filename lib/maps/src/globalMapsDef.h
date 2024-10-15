/**
 * @file globalMapsDef.h
 * @brief  Global Maps Variables
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef GLOBALMAPSDEF_H
#define GLOBALMAPSDEF_H

#include "tft.hpp"

#ifdef LARGE_SCREEN
  #define MAP_HEIGHT 380       // Map Height Size 380
  #define MAP_WIDTH 320        // Map Width Size
  #define MAP_HEIGHT_FULL 480  // Map Height Full Screen
#else
  #ifndef TDECK_ESP32S3
    #define MAP_HEIGHT 220       // Map Height Size
    #define MAP_WIDTH 240        // Map Width Size
    #define MAP_HEIGHT_FULL 320  // Map Height Full Screen
  #endif
  #ifdef TDECK_ESP32S3
    #define MAP_HEIGHT 180       // Map Height Size
    #define MAP_WIDTH 320        // Map Width Size
    #define MAP_HEIGHT_FULL 240  // Map Height Full Screen
  #endif
#endif

#define TILE_HEIGHT 768 // Tile 9x9 Height Size
#define TILE_WIDTH 768  // Tile 9x9 Width Size

#define RENDER_TILE_SIZE 256             // Render Tile Size
#define VECTOR_TILE_SIZE TILE_WIDTH / 2 // Vector Tile Size

extern bool isMapFound;              // Flag to indicate when tile map is found on SD
extern bool isScrolled;              // Flag to indicate when tileview was scrolled
extern bool redrawMap;               // Flag to indicate need redraw Map
extern int wptPosX, wptPosY;         // Waypoint position on render map.

extern TFT_eSprite sprArrow;         // Sprite for Navigation Arrow in map tile
extern TFT_eSprite mapTempSprite;    // Double Buffering Sprites for Map Tile
extern TFT_eSprite mapSprite;        // Double Buffering Sprites for Map Tile
extern uint16_t tileSize;

/**
 * @brief Get pixel X position from OpenStreetMap
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @param tileSize -> tile size
 * @return X position
 */
static uint16_t lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize)
{
  return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Get pixel Y position from OpenStreetMap
 *
 * @param f_lat -> latitude
 * @param zoom -> zoom
 * @param tileSize -> tile size
 * @return Y position
 */
static uint16_t lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize)
{
  return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Structure to store position on screen  of GPS Coordinates
 *
 */
struct ScreenCoord
{
  uint16_t posX;
  uint16_t posY;
};

/**
 * @brief Navigation Arrow position on screen
 *
 */
extern ScreenCoord navArrowPosition;

/**
 * @brief Convert GPS Coordinates to screen position (with offsets)
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> Zoom level
 * @param tileSize -> tile size
 * @return ScreenCoord -> Screen position
 */
static ScreenCoord coord2ScreenPos(double lon, double lat, uint8_t zoomLevel, uint16_t tileSize)
{
  ScreenCoord data;
  data.posX = lon2posx(lon, zoomLevel, tileSize);
  data.posY = lat2posy(lat, zoomLevel, tileSize);
  return data;
}

static const char *noMapFile PROGMEM = "/spiffs/NOMAP.png";

/**
 * @brief Load No Map Image
 *
 */
static void showNoMap(TFT_eSprite &map)
{
  map.drawPngFile(noMapFile, (MAP_WIDTH / 2) - 50, (MAP_HEIGHT / 2) - 50);
  map.drawCenterString("NO MAP FOUND", (MAP_WIDTH / 2), (MAP_HEIGHT >> 1) + 65, &fonts::DejaVu18);
}


/**
 * @brief Structure to store min and max tile latitude and longitude
 *
 */
struct tileBounds 
{
    double lat_min; 
    double lat_max; 
    double lon_min; 
    double lon_max; 
};

/**
 * @brief Render map min and max latitude and longitude
 *
 */
extern tileBounds totalBounds;


#endif
