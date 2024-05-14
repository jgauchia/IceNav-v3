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

#define MAP_HEIGHT 374 // Map Height Size
#define MAP_WIDTH 320  // Map Width Size

static bool refreshMap = false; // Flag to indicate when tile map is found on SD
static bool isMapDraw = false;  // Flag to indicate when maps needs to be draw
static bool isMapFound = true;  // Flag to indicate if map is found
static const char *noMapFile PROGMEM = "/NOMAP.png";

static void showNoMap(TFT_eSprite &map)
{
    map.fillScreen(TFT_BLACK);
    map.drawPngFile(SPIFFS, noMapFile, (MAP_WIDTH / 2) - 50, (MAP_HEIGHT / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (MAP_WIDTH / 2), (MAP_HEIGHT >> 1) + 65, &fonts::DejaVu18);
}

#endif
