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
#include "PsramAllocator.hpp"

/**
 * @brief NAV format constants
 */
static constexpr uint8_t NAV_MAGIC[4] = {'N', 'A', 'V', '1'};

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
