/**
 * @file vectorMaps.hpp
 * @author @aresta - https://github.com/aresta/ESP32_GPS
 * @brief  Vector maps draw functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef VECTORMAPS_HPP
#define VECTORMAPS_HPP

#include <SPI.h>
#include <SD.h>
#include <StreamUtils.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <array>
#include <math.h>
#include <map>
#include "globalMapsDef.h"
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"

const String baseFolder PROGMEM = "/mymap/"; // Vector Map Files Folder
extern bool isPosMoved;                      // Flag when current position changes

/**
 * @brief Vector map object colours
 *
 */
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;
const uint16_t GREEN = 0x76EE;
const uint16_t GREENCLEAR = 0x9F93;
const uint16_t GREENCLEAR2 = 0xCF6E;
const uint16_t BLUE = 0x227E;
const uint16_t BLUECLEAR = 0x6D3E;
const uint16_t CYAN = 0xB7FF;
const uint16_t VIOLET = 0xAA1F;
const uint16_t ORANGE = 0xFCC2;
const uint16_t GRAY = 0x94B2;
const uint16_t GRAYCLEAR = 0xAD55;
const uint16_t GRAYCLEAR2 = 0xD69A;
const uint16_t BROWN = 0xAB00;
const uint16_t YELLOWCLEAR = 0xFFF5;
const uint16_t BACKGROUND_COLOR = 0xEF5D;

/**
 * @brief Vector maps memory definition
 *
 */
#define MAPBLOCKS_MAX 6                                         // max blocks in memory
#define MAPBLOCK_SIZE_BITS 12                                   // 4096 x 4096 coords (~meters) per block
#define MAPFOLDER_SIZE_BITS 4                                   // 16 x 16 map blocks per folder
#define MAX_ZOOM 4                                              // Vector max MAX_ZOOM

const int32_t MAPBLOCK_MASK = pow(2, MAPBLOCK_SIZE_BITS) - 1;   // ...00000000111111111111
const int32_t MAPFOLDER_MASK = pow(2, MAPFOLDER_SIZE_BITS) - 1; // ...00001111

static double lat2y(double lat) { return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS; }
static double lon2x(double lon) { return DEG2RAD(lon) * EARTH_RADIUS; }

/**
 * @brief Point in 16 bits projected coordinates (x,y)
 *
 */
struct Point16
{
  Point16(){};
  Point16(int16_t x, int16_t y) : x(x), y(y){};
  Point16 operator-(const Point16 p) { return Point16(x - p.x, y - p.y); };
  Point16 operator+(const Point16 p) { return Point16(x + p.x, y + p.y); };
  Point16(char *coordsPair); // char array like:  11.222,333.44
  int16_t x;
  int16_t y;
};

/**
 * @brief Point in 32 bits projected coordinates (x,y)
 *
 */
struct Point32
{
  Point32(){};
  Point32(int32_t x, int32_t y) : x(x), y(y){};
  Point32(Point16 p) : x(p.x), y(p.y){};
  Point32 operator-(const Point32 p) { return Point32(x - p.x, y - p.y); };
  Point32 operator+(const Point32 p) { return Point32(x + p.x, y + p.y); };
  Point16 toPoint16() { return Point16(x, y); }; // TODO: check limits
  bool operator==(const Point32 p) { return x == p.x && y == p.y; };

  /// @brief Parse char array with the coordinates
  /// @param coordsPair char array like:  11.222,333.44

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
  // @brief Bounding Box
  // @param min top left corner
  // @param max bottim right corner
  BBox(Point32 min, Point32 max) : min(min), max(max){};
  BBox operator-(const Point32 p) { return BBox(min - p, max - p); };
  bool containsPoint(const Point32 p);
  bool intersects(const BBox b);
  Point32 min;
  Point32 max;
};

/**
 * @brief Polyline
 *
 */
struct Polyline
{
  std::vector<Point16> points;
  BBox bbox;
  uint16_t color;
  uint8_t width;
  uint8_t maxZoom;
};

/**
 * @brief Polygon
 *
 */
struct Polygon
{
  std::vector<Point16> points;
  BBox bbox;
  uint16_t color;
  uint8_t maxZoom;
};

/**
 * @brief Vector map viewport
 *
 */
struct ViewPort
{
  void setCenter(Point32 pcenter);
  Point32 center;
  BBox bbox;
};

int16_t toScreenCoord(const int32_t pxy, const int32_t screenCenterxy);

/**
 * @brief Map square area of aprox 4096 meters side. Correspond to one single map file.
 *
 */
struct MapBlock
{
  Point32 offset;
  // BBox bbox;
  bool inView = false;
  std::vector<Polyline> polylines;
  std::vector<Polygon> polygons;
};

/**
 * @brief MapBlocs memory store
 *
 */
struct MemCache
{
  std::vector<MapBlock *> blocks;
};

/**
 * @brief MapBlocks stored in memory
 *
 */
struct MemBlocks
{
  std::map<String, u_int16_t> blocks_map; // block offset -> block index
  std::array<MapBlock *, MAPBLOCKS_MAX> blocks;
};

/**
 * @brief Point in geografic (lat,lon) coordinates and other gps data
 *
 */
struct Coord
{
  Point32 getPoint32();
  double lat = 0;
  double lng = 0;
  int16_t altitude = 0;
  int16_t direction = 0;
  int16_t satellites = 0;
  bool isValid = false;
  bool isUpdated = false;
};

extern MemBlocks memBlocks;     // Vector file map memory blocks
extern ViewPort viewPort;       // Vector map viewport
extern MemCache memCache;       // Memory Cache
extern Point32 point;           // Vector map GPS position point
extern double prevLat, prevLng; // Previous Latitude and Longitude

void getPosition(double lat, double lon);
int16_t parseInt16(ReadBufferingStream &file);
void parseStrUntil(ReadBufferingStream &file, char terminator, char *str);
void parseCoords(ReadBufferingStream &file, std::vector<Point16> &points);
BBox parseBbox(String str);
MapBlock *readMapBlock(String fileName);
void getMapBlocks(BBox &bbox, MemCache &memCache);
void fillPolygon(Polygon p, TFT_eSprite &map); // scanline fill algorithm
void generateVectorMap(ViewPort &viewPort, MemCache &memCache, TFT_eSprite &map);



#endif
