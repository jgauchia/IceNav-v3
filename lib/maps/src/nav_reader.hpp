/**
 * @file nav_reader.hpp
 * @brief NAV tile reader for ESP32 - IceNav Navigation Tiles
 * @version 0.3.0
 * @date 2026-06
 *
 * NPK2 pack: MapHeader (23B) + sparse index (u32 count, coverage bitmap, popcount rank
 * table, compact 8B entries) + global RGB565 color palette + tile data. Coverage bitmap
 * has one bit per grid cell; a rank table (u32 per 64 bitmap bytes = 512 cells) gives the
 * compact-entry position in O(1). Each tile has a 6-byte header; features use an 8-byte
 * fixed header (1-byte palette color index) followed by varint coord_count and payload_size.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include "esp_heap_caps.h"
#include "PsramAllocator.hpp"

static constexpr uint8_t NAV_PACK_HDR_SIZE           = 23;

static constexpr uint8_t NAV_SPARSE_COUNT_SIZE       = 4;
static constexpr uint8_t NAV_RANK_STRIDE_BYTES       = 64;

static constexpr uint8_t NAV_TILE_HDR_FEAT_COUNT_OFF = 4;
static constexpr uint8_t NAV_TILE_HDR_SIZE           = 6;

static constexpr uint8_t NAV_FEAT_GEOM_OFF           = 0;
static constexpr uint8_t NAV_FEAT_COLOR_IDX_OFF      = 1;
static constexpr uint8_t NAV_FEAT_ZP_OFF             = 2;
static constexpr uint8_t NAV_FEAT_WP_OFF             = 3;
static constexpr uint8_t NAV_FEAT_BX1_OFF            = 4;
static constexpr uint8_t NAV_FEAT_BY1_OFF            = 5;
static constexpr uint8_t NAV_FEAT_BX2_OFF            = 6;
static constexpr uint8_t NAV_FEAT_BY2_OFF            = 7;
static constexpr uint8_t NAV_FEAT_HDR_FIXED_SIZE     = 8;

enum class NavGeomType : uint8_t
{
    Point = 1,
    LineString = 2,
    Polygon = 3,
    Text = 4
};

class NavReader
{
public:
    static FILE* packFile;
    static void closePack();
    static bool openPack(uint8_t zoom);
    static bool findTileInPack(uint32_t tileX, uint32_t tileY, uint32_t& offset, uint32_t& size);

    struct IndexEntry
    {
        uint32_t offset;
        uint32_t size;
    };

    static inline uint16_t paletteColor(uint8_t index)
    {
        return index < paletteCount ? colorPalette[index] : 0xFFFF;
    }

    static inline uint32_t readVarIntU(const uint8_t*& p)
    {
        uint32_t result = 0;
        int shift = 0;
        while (true)
        {
            uint8_t byte = *p++;
            result |= (uint32_t)(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0)
                break;
            shift += 7;
        }
        return result;
    }

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

private:
    static uint8_t  currentZoom;
    static uint32_t tilesWide;
    static uint32_t tilesHigh;
    static uint32_t minX;
    static uint32_t minY;

    static uint32_t indexBase;
    static uint32_t rankBase;
    static uint32_t entriesBase;
    static uint32_t bitmapBytes;
    static uint32_t indexCount;

    static uint16_t* colorPalette;
    static uint16_t  paletteCount;
};
