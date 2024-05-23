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

extern bool isMapFound;              // Flag to indicate when tile map is found on SD
extern bool isScrolled;              // Flag to indicate when tileview was scrolled+
extern TFT_eSprite sprArrow;         // Sprite for Navigation Arrow in map tile
extern TFT_eSprite mapTempSprite;    // Double Buffering Sprites for Map Tile
extern TFT_eSprite mapSprite;        // Double Buffering Sprites for Map Tile

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
 * @brief Navitagion Arrow position on screen
 *
 */
extern ScreenCoord navArrowPosition;


static const char *noMapFile PROGMEM = "/NOMAP.png";

/**
 * @brief Load No Map Image
 *
 */
static void showNoMap(TFT_eSprite &map)
{
    map.drawPngFile(SPIFFS, noMapFile, (MAP_WIDTH / 2) - 50, (MAP_HEIGHT / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (MAP_WIDTH / 2), (MAP_HEIGHT >> 1) + 65, &fonts::DejaVu18);
}

#endif
