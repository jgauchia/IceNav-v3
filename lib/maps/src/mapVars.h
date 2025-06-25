/**
 * @file mapsVars.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Maps vars and structures
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include "bruj.h"
#include "compass.h"
#include "waypoint.h"
#include "navfinish.h"
#include "straight.h"
#include "slleft.h"
#include "slright.h"
#include "tleft.h"
#include "tright.h"
#include "uleft.h"
#include "uright.h"
#include "finish.h"

#include "globalGpxDef.h"

/**
 * @brief Point in 16 bits projected coordinates (x, y) for Vector Maps.
 *
 * This struct represents a point using 16-bit signed integers for the x and y coordinates.
 * It provides basic arithmetic operators for point addition and subtraction,
 * and a constructor for parsing a coordinate pair from a C-string.
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
 * @brief Point in 32 bits projected coordinates (x, y) for Vector Maps.
 *
 * This struct represents a point using 32-bit signed integers for the x and y coordinates.
 * It provides constructors for direct initialization and from Point16,
 * arithmetic operators for point addition and subtraction, a conversion to Point16,
 * and equality comparison.
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
 * @brief Bounding Box structure for 2D space using Point32.
 *
 * Represents an axis-aligned bounding box with minimum and maximum corners.
 * Provides methods for translation, point containment, and intersection tests.
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

static const String mapVectorFolder PROGMEM = "/sdcard/VECTMAP/";        /**< Vector Map Files Folder */
static const char *mapRenderFolder PROGMEM = "/sdcard/MAP/%u/%u/%u.png"; /**< Render Maps file folder */
static const char *noMapFile PROGMEM = "/spiffs/NOMAP.png";              /**< No map image file */
static const char *map_scale[] PROGMEM = {"5000 Km", "2500 Km", "1500 Km",
										"700 Km", "350 Km", "150 Km",
										"100 Km", "40 Km", "20 Km",
										"10 Km", "5 Km", "2,5 Km",
										"1,5 Km", "700 m", "350 m",
										"150 m", "80 m", "40 m",
										"20 m", "10 m"
										}; /**< Scale label for map */



/**
 * @brief Vector map object colours
 *
 */
static const uint16_t WHITE = 0xFFFF;           /**< Vector map object colour: White */
static const uint16_t BLACK = 0x0000;           /**< Vector map object colour: Black */
static const uint16_t GREEN = 0x76EE;           /**< Vector map object colour: Green */
static const uint16_t GREENCLEAR = 0x9F93;      /**< Vector map object colour: Green Clear */
static const uint16_t GREENCLEAR2 = 0xCF6E;     /**< Vector map object colour: Green Clear 2 */
static const uint16_t BLUE = 0x227E;            /**< Vector map object colour: Blue */
static const uint16_t BLUECLEAR = 0x6D3E;       /**< Vector map object colour: Blue Clear */
static const uint16_t CYAN = 0xB7FF;            /**< Vector map object colour: Cyan */
static const uint16_t VIOLET = 0xAA1F;          /**< Vector map object colour: Violet */
static const uint16_t ORANGE = 0xFCC2;          /**< Vector map object colour: Orange */
static const uint16_t GRAY = 0x94B2;            /**< Vector map object colour: Gray */
static const uint16_t GRAYCLEAR = 0xAD55;       /**< Vector map object colour: Gray Clear */
static const uint16_t GRAYCLEAR2 = 0xD69A;      /**< Vector map object colour: Gray Clear 2 */
static const uint16_t BROWN = 0xAB00;           /**< Vector map object colour: Brown */
static const uint16_t YELLOWCLEAR = 0xFFF5;     /**< Vector map object colour: Yellow Clear */
static const uint16_t BACKGROUND_COLOR = 0xEF5D;/**< Vector map object colour: Background */

/**
 * @brief Vector maps memory definition
 *
 */
#define MAPBLOCKS_MAX 6            /**< max blocks in memory */
#define MAPBLOCK_SIZE_BITS 12      /**< 4096 x 4096 coords (~meters) per block */
#define MAPFOLDER_SIZE_BITS 4      /**< 16 x 16 map blocks per folder */
#define MAX_ZOOM 4                 /**< Vector max MAX_ZOOM */
          