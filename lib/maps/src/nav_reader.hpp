/**
 * @file nav_reader.hpp
 * @brief NAV tile reader for ESP32 - IceNav Navigation Tiles
 * @version 1.0.0
 * @date 2025-01
 *
 * NAV format uses int32 coordinates (scaled by 1e7) for compact storage.
 * Simple binary format optimized for ESP32 sequential reading.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include "esp_heap_caps.h"

/**
 * @brief NAV format constants
 */
static constexpr uint8_t NAV_MAGIC[4] = {'N', 'A', 'V', '1'};
static constexpr int32_t COORD_SCALE = 10000000;  // 1e7

/**
 * @brief Geometry types
 */
enum class NavGeomType : uint8_t
{
    Point = 1,
    LineString = 2,
    Polygon = 3
};

#pragma pack(push, 1)
/**
 * @brief NAV Tile Header (22 bytes)
 */
struct NavTileHeader
{
    char magic[4];          // "NAV1"
    uint16_t featureCount;  // Little-Endian
    int32_t minLon;         // scaled 1e7
    int32_t minLat;
    int32_t maxLon;
    int32_t maxLat;
};

/**
 * @brief NAV Feature Header (12 bytes aligned)
 */
struct NavFeatureHeader
{
    uint8_t geomType;       // 1=Point, 2=LineString, 3=Polygon
    uint16_t colorRgb565;   // Little-Endian
    uint8_t zoomPriority;   // High: min_zoom, Low: priority
    uint8_t widthPixels;    // 1-15
    uint8_t bbox[4];        // [x1, y1, x2, y2] normalized 0-255
    uint16_t coordCount;    // Total points
    uint16_t payloadSize;   // Total bytes of payload (coords + rings)
};
#pragma pack(pop)

/**
 * @brief Bounding box (int32 scaled coordinates)
 */
struct NavBbox
{
    int32_t minLon;
    int32_t minLat;
    int32_t maxLon;
    int32_t maxLat;

    // Convert to double for comparisons
    double minLonF() const { return static_cast<double>(minLon) / COORD_SCALE; }
    double minLatF() const { return static_cast<double>(minLat) / COORD_SCALE; }
    double maxLonF() const { return static_cast<double>(maxLon) / COORD_SCALE; }
    double maxLatF() const { return static_cast<double>(maxLat) / COORD_SCALE; }
};

/**
 * @brief Object-level Bounding Box (normalized 0-255)
 */
struct NavObjBbox
{
    uint8_t x1, y1, x2, y2;
};

/**
 * @brief Single coordinate (int16 relative to tile 0-4096)
 */
struct NavCoord
{
    int16_t x;
    int16_t y;
};

/**
 * @brief Feature properties
 */
struct NavProperties
{
    uint16_t colorRgb565;
    uint8_t zoomPriority;  // High nibble = min_zoom, low nibble = priority
    uint8_t width;         // Line width in pixels

    uint8_t getMinZoom() const { return zoomPriority >> 4; }
    uint8_t getPriority() const { return (zoomPriority & 0x0F); }
};

/**
 * @brief Parsed NAV feature
 */
struct NavFeature
{
    NavGeomType geomType;
    NavProperties properties;
    NavObjBbox objBbox;
    std::vector<NavCoord> coords;
    uint16_t coordCount = 0;
    std::vector<uint16_t> ringEnds;
    uint16_t ringCount = 0;
    
    // Pixel offset of the tile top-left relative to viewport top-left
    int16_t tilePixelOffsetX;
    int16_t tilePixelOffsetY;
};

/**
 * @brief NAV tile reader
 *
 * Efficient memory-based reader for NAV binary tiles.
 */
class NavReader
{
public:
    /**
     * @brief Load entire file to memory and read features
     * @param path File path
     * @param features Output features
     * @param maxZoom Zoom filter
     * @param tileBuffer Buffer to store file data (must be pre-allocated or will be allocated)
     * @param bufferSize Size of tileBuffer
     */
    static size_t readAllFeaturesMemory(const char* path, std::vector<NavFeature>& features, uint8_t maxZoom, uint8_t*& tileBuffer, size_t& bufferSize);
};
