/**
 * @file nav_reader.hpp
 * @brief NAV tile reader for ESP32 - IceNav Navigation Tiles
 * @version 0.2.6
 * @date 2026-05
 *
 * NAV format uses int32 coordinates (scaled by 1e7) for compact storage.
 * Simple binary format optimized for ESP32 sequential reading.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include "esp_heap_caps.h"
#include "PsramAllocator.hpp"

/**
 * @brief NAV format constants
 */
static constexpr uint8_t NAV_MAGIC[4] = {'N', 'A', 'V', '1'};

// NAV tile binary format offsets (tile header)
static constexpr uint8_t NAV_TILE_HDR_FEAT_COUNT_OFF = 4;   ///< 2 bytes: feature count
static constexpr uint8_t NAV_TILE_HDR_SIZE           = 22;  ///< total tile header size

// NAV feature record offsets (relative to feature start pointer)
static constexpr uint8_t NAV_FEAT_GEOM_OFF           = 0;   ///< 1 byte: geometry type
static constexpr uint8_t NAV_FEAT_COLOR_OFF          = 1;   ///< 2 bytes: RGB565 color
static constexpr uint8_t NAV_FEAT_ZP_OFF             = 3;   ///< 1 byte: zoom+priority packed
static constexpr uint8_t NAV_FEAT_WP_OFF             = 4;   ///< 1 byte: width+casing packed
static constexpr uint8_t NAV_FEAT_BX1_OFF            = 5;   ///< 1 byte: bounding box x1
static constexpr uint8_t NAV_FEAT_BY1_OFF            = 6;   ///< 1 byte: bounding box y1
static constexpr uint8_t NAV_FEAT_BX2_OFF            = 7;   ///< 1 byte: bounding box x2
static constexpr uint8_t NAV_FEAT_BY2_OFF            = 8;   ///< 1 byte: bounding box y2
static constexpr uint8_t NAV_FEAT_COORD_COUNT_OFF    = 9;   ///< 2 bytes: coordinate count
static constexpr uint8_t NAV_FEAT_PAYLOAD_SIZE_OFF   = 11;  ///< 2 bytes: payload size
static constexpr uint8_t NAV_FEAT_HDR_SIZE           = 13;  ///< total feature header size

/**
 * @brief Geometry types
 */
enum class NavGeomType : uint8_t
{
    Point = 1,
    LineString = 2,
    Polygon = 3,
    Text = 4
};

/**
 * @brief NAV tile reader
 *
 * Efficient memory-based reader for NAV binary tiles.
 */
class NavReader
{
public:
    static FILE* packFile;
    static void closePack();
    static bool openPack(uint8_t zoom);
    static bool findTileInPack(uint32_t tileX, uint32_t tileY, uint32_t& offset, uint32_t& size);

    /**
     * @brief Convert (x,y) tile coordinates to Hilbert index.
     */
    static uint64_t xyToHilbert(uint32_t x, uint32_t y, uint8_t z)
    {
        uint32_t rx, ry;
        uint64_t d = 0;
        uint32_t n = 1 << z;
        for (uint32_t s = n / 2; s > 0; s /= 2)
        {
            rx = (x & s) > 0;
            ry = (y & s) > 0;
            d += static_cast<uint64_t>(s) * s * ((3 * rx) ^ ry);
            hilbertRot(s, &x, &y, rx, ry);
        }
        return d;
    }

private:
    static uint8_t currentZoom;
    static uint32_t tileCount;
    static uint32_t indexOff;

    static void hilbertRot(uint32_t n, uint32_t* x, uint32_t* y, uint32_t rx, uint32_t ry)
    {
        if (ry == 0)
        {
            if (rx == 1)
            {
                *x = n - 1 - *x;
                *y = n - 1 - *y;
            }
            uint32_t t = *x;
            *x = *y;
            *y = t;
        }
    }

public:
    static inline int32_t readVarInt(uint8_t*& p)
    {
        int32_t result = 0;
        int shift = 0;
        while (true)
        {
            uint8_t byte = *p++;
            result |= (byte & 0x7F) << shift;
            if ((byte & 0x80) == 0)
                break;
            shift += 7;
        }
        return result;
    }

    static inline int32_t decodeZigZag(int32_t n)
    {
        return (n >> 1) ^ -(n & 1);
    }
};
