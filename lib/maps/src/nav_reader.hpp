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
    std::vector<NavCoord> coords;
    std::vector<uint16_t> ringEnds;  // For polygons
};

/**
 * @brief NAV tile header
 */
struct NavHeader
{
    uint16_t featureCount;
    NavBbox bbox;
};

/**
 * @brief NAV tile reader
 *
 * Simple sequential reader for NAV binary tiles.
 * No spatial index needed - tiles are small and read entirely.
 */
class NavReader
{
public:
    NavReader();
    ~NavReader();

    /**
     * @brief Open and read NAV tile
     * @param path Path to .nav file
     * @return true if successful
     */
    bool open(const char* path);

    /**
     * @brief Close file handle
     */
    void close();

    /**
     * @brief Check if file is open
     */
    bool isOpen() const { return file_ != nullptr; }

    /**
     * @brief Get tile header
     */
    const NavHeader& getHeader() const { return header_; }

    /**
     * @brief Read features from tile with filtering
     * @param features Output vector
     * @param maxZoom Only include features with min_zoom <= maxZoom
     * @param viewport Optional viewport for culling (nullptr to disable)
     * @return Number of features read
     */
    size_t readAllFeatures(std::vector<NavFeature>& features, uint8_t maxZoom, const NavBbox* viewport = nullptr);

    /**
     * @brief Get bytes read (for statistics)
     */
    size_t getBytesRead() const { return bytesRead_; }

    /**
     * @brief Reset bytes counter
     */
    void resetBytesRead() { bytesRead_ = 0; }

private:
    FILE* file_;
    NavHeader header_;
    size_t bytesRead_;
    bool headerValid_;

    bool readHeader();
    bool readFeature(NavFeature& feature);
};
