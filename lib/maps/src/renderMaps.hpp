/**
 * @file renderMaps.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Render maps draw functions
 * @version 0.1.8
 * @date 2024-04
 */

#ifndef RENDERMAPS_HPP
#define RENDERMAPS_HPP

#include <Arduino.h>
#include "tft.hpp"
#include "compass.hpp"
#include "settings.hpp"
#include "globalMapsDef.h"

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

static const char *mapFolder PROGMEM = "/MAP/%d/%d/%d.png"; // Render Maps file folder

extern MapTile oldMapTile;     // Old Map tile coordinates and zoom
extern MapTile currentMapTile; // Curreng Map tile coordinates and zoom
extern MapTile roundMapTile;   // Boundaries Map tiles

extern uint16_t tileSize;
extern TFT_eSprite sprArrow;     // Sprite for Navigation Arrow in map tile
extern TFT_eSprite mapSprite;    // Double Buffering Sprites for Map Tile
extern TFT_eSprite mapRotSprite; // Double Buffering Sprites for Map Tile

uint32_t lon2tilex(double f_lon, uint8_t zoom);
uint32_t lat2tiley(double f_lat, uint8_t zoom);
uint16_t lon2posx(float f_lon, uint8_t zoom);
uint16_t lat2posy(float f_lat, uint8_t zoom);
ScreenCoord coord2ScreenPos(double lon, double lat, uint8_t zoomLevel);
MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY);
void generateRenderMap();

#endif
