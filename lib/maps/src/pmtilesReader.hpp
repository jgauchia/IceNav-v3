/**
 * @file pmtilesReader.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  PMTiles v3 reader for the optional MVT base map
 * @version 0.3.3
 * @date 2026-11
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

// PMTiles v3 reader: resolves (z, x, y) to a byte range inside a single archive
// on the SD card. Directories are small and are decompressed in memory; tile
// bodies stay compressed and are inflated by the caller.
class PmtilesReader
{
public:
    struct Entry
    {
        uint64_t tileId;
        uint64_t offset;
        uint32_t length;
        uint32_t runLength;
    };

    static bool openPack(uint8_t zoom);
    static void closePack();
    static bool isOpen();
    static bool findTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, uint64_t& offset, uint32_t& length);
    static bool readTile(uint64_t offset, uint32_t length, uint8_t* out, size_t outCapacity, size_t& outSize);
    static uint64_t tileIdFromZxy(uint8_t zoom, uint32_t tileX, uint32_t tileY);
    static const char* lastError();
    static void releaseBuffers();

private:
    static bool readDirectory(uint64_t offset, uint64_t length, std::vector<Entry>& out);
    static bool gunzip(const uint8_t* src, size_t srcLength, uint8_t* dst, size_t dstCapacity, size_t& dstLength);
    static bool ensureCompressedBuffer(size_t bytes);
    static bool ensureInflateBuffer(size_t bytes);
    static const Entry* lastAtOrBefore(const std::vector<Entry>& entries, uint64_t tileId);

    static FILE* packFile;
    static bool opened;
    static uint8_t zoomLevel;
    static uint64_t rootOffset;
    static uint64_t rootLength;
    static uint64_t leafOffset;
    static uint64_t tileOffset;
    static uint8_t internalCompression;
    static uint8_t tileCompression;
    static std::vector<Entry> rootEntries;
    static std::vector<Entry> leafEntries;
    static uint64_t leafFirstId;
    static bool leafValid;
    static uint8_t* compressedBuffer;
    static size_t compressedCapacity;
    static uint8_t* inflateBuffer;
    static size_t inflateCapacity;
    static const char* errorText;
};
