/**
 * @file renderMaps.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Render maps draw functions
 * @version 0.1.8
 * @date 2024-06
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
extern MapTile currentMapTile; // Curreng Map tile coordinates and zoom
extern MapTile roundMapTile;   // Boundaries Map tiles

uint32_t lon2tilex(double f_lon, uint8_t zoom);
uint32_t lat2tiley(double f_lat, uint8_t zoom);
MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY);
void drawMapWidgets();
void generateRenderMap();

#endif
