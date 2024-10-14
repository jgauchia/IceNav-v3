/**
 * @file renderMaps.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Render maps draw functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef RENDERMAPS_HPP
#define RENDERMAPS_HPP

#include <Arduino.h>
#include "tft.hpp"
#include "compass.hpp"
#include "settings.hpp"
#include "globalMapsDef.h"
#include "mapsDrawFunc.h"

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
extern MapTile currentMapTile; // Current Map tile coordinates and zoom
extern MapTile roundMapTile;   // Boundaries Map tiles

extern double destLat;
extern double destLon;

uint32_t lon2tilex(double f_lon, uint8_t zoom);
uint32_t lat2tiley(double f_lat, uint8_t zoom);
double tilex2lon(uint32_t tileX, uint8_t zoom);
double tiley2lat(uint32_t tileY, uint8_t zoom);
tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom);
bool isCoordInBounds(double lat, double lon, tileBounds bound);
void coords2map(double lat, double lon, tileBounds bound, int *pixelX, int *pixelY);
MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY);
void drawMapWidgets();
void generateRenderMap();
void initSD();
void acquireSdSPI();
void releaseSdSPI();

#endif
