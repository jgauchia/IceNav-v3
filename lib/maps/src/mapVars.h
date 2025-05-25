/**
 * @file mapsVars.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Maps vars and structures
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

 /**
 * @brief Map Widgets images.
 *
 */
#include "bruj.h"
#include "navigation.h"
#include "waypoint.h"
#include "compass.h"
#include "zoom.h"
#include "speed.h"
#include "expand.h"
#include "collapse.h"
#include "zoomin.h"
#include "zoomout.h"
#include "navfinish.h"
#include "move.h"

#include "globalGpxDef.h"

/**
 * @brief Point in 16 bits projected coordinates (x,y) Vector Maps
 *
 */
struct Point16
{
  Point16(){};
  Point16(int16_t x, int16_t y) : x(x), y(y){};
  Point16 operator-(const Point16 p) { return Point16(x - p.x, y - p.y); };
  Point16 operator+(const Point16 p) { return Point16(x + p.x, y + p.y); };
  Point16(char *coordsPair); 
  int16_t x;
  int16_t y;
};

/**
 * @brief Point in 32 bits projected coordinates (x,y) Vector Maps
 *
 */
struct Point32
{
  Point32(){};
  Point32(int32_t x, int32_t y) : x(x), y(y){};
  Point32(Point16 p) : x(p.x), y(p.y){};
  Point32 operator-(const Point32 p) { return Point32(x - p.x, y - p.y); };
  Point32 operator+(const Point32 p) { return Point32(x + p.x, y + p.y); };
  Point16 toPoint16() { return Point16(x, y); };
  bool operator==(const Point32 p) { return x == p.x && y == p.y; };
  int32_t x;
  int32_t y;
};

/**
 * @brief Bounding Box
 *
 */
struct BBox
{
  BBox(){};
  BBox(Point32 min, Point32 max) : min(min), max(max){};
  BBox operator-(const Point32 p) { return BBox(min - p, max - p); };
  bool containsPoint(const Point32 p);
  bool intersects(const BBox b);
  Point32 min;
  Point32 max;
};

static const String mapVectorFolder PROGMEM = "/sdcard/VECTMAP/";        // Vector Map Files Folder
static const char *mapRenderFolder PROGMEM = "/sdcard/MAP/%u/%u/%u.png"; // Render Maps file folder
static const char *noMapFile PROGMEM = "/spiffs/NOMAP.png";       // No map image file
static const char *map_scale[] PROGMEM = {"5000 Km", "2500 Km", "1500 Km",
                                          "700 Km", "350 Km", "150 Km",
                                          "100 Km", "40 Km", "20 Km",
                                          "10 Km", "5 Km", "2,5 Km",
                                          "1,5 Km", "700 m", "350 m",
                                          "150 m", "80 m", "40 m",
                                          "20 m", "10 m"};       // Scale label for map


/**
 * @brief Vector map object colours
 *
 */
static const uint16_t WHITE = 0xFFFF;
static const uint16_t BLACK = 0x0000;
static const uint16_t GREEN = 0x76EE;
static const uint16_t GREENCLEAR = 0x9F93;
static const uint16_t GREENCLEAR2 = 0xCF6E;
static const uint16_t BLUE = 0x227E;
static const uint16_t BLUECLEAR = 0x6D3E;
static const uint16_t CYAN = 0xB7FF;
static const uint16_t VIOLET = 0xAA1F;
static const uint16_t ORANGE = 0xFCC2;
static const uint16_t GRAY = 0x94B2;
static const uint16_t GRAYCLEAR = 0xAD55;
static const uint16_t GRAYCLEAR2 = 0xD69A;
static const uint16_t BROWN = 0xAB00;
static const uint16_t YELLOWCLEAR = 0xFFF5;
static const uint16_t BACKGROUND_COLOR = 0xEF5D;

/**
 * @brief Vector maps memory definition
 *
 */
#define MAPBLOCKS_MAX 6                                                // max blocks in memory
#define MAPBLOCK_SIZE_BITS 12                                          // 4096 x 4096 coords (~meters) per block
#define MAPFOLDER_SIZE_BITS 4                                          // 16 x 16 map blocks per folder
#define MAX_ZOOM 4                                                     // Vector max MAX_ZOOM                              