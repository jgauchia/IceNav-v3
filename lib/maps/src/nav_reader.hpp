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
 * @brief Single coordinate (int32 scaled)
 */
struct NavCoord
{
    int32_t lon;
    int32_t lat;

    // Convert to double
    double lonF() const { return static_cast<double>(lon) / COORD_SCALE; }
    double latF() const { return static_cast<double>(lat) / COORD_SCALE; }
};

/**
 * @brief Feature properties
 */
struct NavProperties
{
    uint16_t colorRgb565;
    uint8_t zoomPriority;  // High nibble = min_zoom, low nibble = priority/7
    uint8_t width;         // Line width in pixels (NAV v2, default 1)

    uint8_t getMinZoom() const { return zoomPriority >> 4; }
    uint8_t getPriority() const { return (zoomPriority & 0x0F); }
    uint8_t getWidth() const { return width > 0 ? width : 1; }
};

/**
 * @brief Parsed NAV feature
 */
struct NavFeature
{
    NavGeomType geomType;
    NavProperties properties;
    NavCoord* coords = nullptr;
    uint16_t coordCount = 0;
    uint16_t* ringEnds = nullptr;
    uint8_t ringCount = 0;
    
    // Feature bounding box for fast culling
    NavBbox bbox;
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
