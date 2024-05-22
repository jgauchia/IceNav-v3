/**
 * @file globalMapsDef.h
 * @brief  Global Maps Variables
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef GLOBALMAPSDEF_H
#define GLOBALMAPSDEF_H

#include "SPIFFS.h"
#include "tft.hpp"

#define MAP_HEIGHT 374  // Map Height Size
#define MAP_WIDTH 320   // Map Width Size
#define TILE_HEIGHT 768 // Tile 9x9 Height Size
#define TILE_WIDTH 768  // Tile 9x9 Width Size

extern bool refreshMap;         // Flag to indicate when maps needs to be refresh
extern bool isMapDraw;          // Flag to indicate when maps needs to be draw
extern bool isMapFound;         // Flag to indicate when tile map is found on SD
extern bool isScrolled;         // Flag to indicate when tileview was scrolled

static const char *noMapFile PROGMEM = "/NOMAP.png";

static void showNoMap(TFT_eSprite &map)
{
    map.drawPngFile(SPIFFS, noMapFile, (MAP_WIDTH / 2) - 50, (MAP_HEIGHT / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (MAP_WIDTH / 2), (MAP_HEIGHT >> 1) + 65, &fonts::DejaVu18);
}

#endif
