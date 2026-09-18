/**
 * @file nav_reader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Binary NAV file reader and tile container manager
 * @version 0.3.0
 * @date 2026-09
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "storage.hpp"
#include "mapVars.h"

extern Storage storage;
static const char* TAG = "NAVREADER";

FILE*    NavReader::packFile    = nullptr;
uint8_t  NavReader::currentZoom = 0;
uint32_t NavReader::tilesWide   = 0;
uint32_t NavReader::tilesHigh   = 0;
uint32_t NavReader::minX        = 0;
uint32_t NavReader::minY        = 0;

uint32_t NavReader::indexBase    = 0;
uint32_t NavReader::rankBase     = 0;
uint32_t NavReader::entriesBase  = 0;
uint32_t NavReader::bitmapBytes  = 0;
uint32_t NavReader::indexCount   = 0;

uint16_t* NavReader::colorPalette = nullptr;
uint16_t  NavReader::paletteCount = 0;

// RAM index cache: keeps sparse-index lookups (bitmap block + rank + entry)
// off the SD card across viewport rows. Falls back to plain per-read lookups
// when no PSRAM/internal RAM is available.
static constexpr uint8_t IDX_CACHE_BLKS    = 16;
static constexpr uint8_t IDX_CACHE_ENTRIES = 64;
static uint8_t*   idxBlkMem   = nullptr;
static uint32_t*  idxBlkTag   = nullptr;
static uint32_t*  idxRankMem  = nullptr;
static uint32_t*  idxRankTag  = nullptr;
static NavReader::IndexEntry* idxEntryMem = nullptr;
static uint64_t*  idxEntryTag = nullptr;
static uint8_t   idxBlkFresh   = 0;
static uint8_t   idxRankFresh  = 0;

/**
 * @brief Allocate memory preferring PSRAM, falling back to internal RAM.
 *
 * @param bytes Byte count to allocate.
 * @return Pointer to the buffer, or nullptr on failure.
 */
static void* allocIndexCacheRam(size_t bytes)
{
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!p)
        p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL);
    return p;
}

/**
 * @brief Release every index-cache buffer.
 */
static void freeIndexCache()
{
    heap_caps_free(idxBlkMem);
    heap_caps_free(idxBlkTag);
    heap_caps_free(idxRankMem);
    heap_caps_free(idxRankTag);
    heap_caps_free(idxEntryMem);
    heap_caps_free(idxEntryTag);
    idxBlkMem   = nullptr;
    idxBlkTag   = nullptr;
    idxRankMem  = nullptr;
    idxRankTag  = nullptr;
    idxEntryMem = nullptr;
    idxEntryTag = nullptr;
    idxBlkFresh  = 0;
    idxRankFresh = 0;
}

/**
 * @brief Allocate and initialize the sparse-index cache.
 *
 * Invalidate the tags so lookups miss until their block loads. When any
 * allocation fails the whole cache is dropped and per-read lookups remain.
 */
static void allocateIndexCache()
{
    freeIndexCache();
    idxBlkMem   = static_cast<uint8_t*>(allocIndexCacheRam(IDX_CACHE_BLKS * NAV_RANK_STRIDE_BYTES));
    idxBlkTag   = static_cast<uint32_t*>(allocIndexCacheRam(IDX_CACHE_BLKS * sizeof(uint32_t)));
    idxRankMem  = static_cast<uint32_t*>(allocIndexCacheRam(IDX_CACHE_BLKS * sizeof(uint32_t)));
    idxRankTag  = static_cast<uint32_t*>(allocIndexCacheRam(IDX_CACHE_BLKS * sizeof(uint32_t)));
    idxEntryMem = static_cast<NavReader::IndexEntry*>(allocIndexCacheRam(IDX_CACHE_ENTRIES * sizeof(NavReader::IndexEntry)));
    idxEntryTag = static_cast<uint64_t*>(allocIndexCacheRam(IDX_CACHE_ENTRIES * sizeof(uint64_t)));
    if (!idxBlkMem || !idxBlkTag || !idxRankMem || !idxRankTag || !idxEntryMem || !idxEntryTag)
    {
        freeIndexCache();
        return;
    }
    for (uint8_t i = 0; i < IDX_CACHE_BLKS; i++)
    {
        idxBlkTag[i]   = UINT32_MAX;
        idxRankTag[i]  = UINT32_MAX;
    }
    for (uint8_t i = 0; i < IDX_CACHE_ENTRIES; i++)
        idxEntryTag[i] = UINT64_MAX;
    idxBlkFresh   = 0;
    idxRankFresh  = 0;
}

/**
 * @brief Open a packed tile container for the given zoom level.
 *
 * @param zoom Zoom level.
 * @return True if successful.
 */
bool NavReader::openPack(uint8_t zoom)
{
    if (packFile && currentZoom == zoom)
        return true;

    closePack();

    char path[64];
    snprintf(path, sizeof(path), mapVectorFolder, zoom);
    packFile = storage.open(path, "rb");
    if (!packFile)
        return false;

    char magic[4];
    if (storage.readDirect(packFile, (uint8_t*)magic, 4) != 4 || memcmp(magic, "NPK2", 4) != 0)
    {
        ESP_LOGE(TAG, "Invalid packed magic for %s", path);
        closePack();
        return false;
    }

    uint8_t fileZoom;
    if (storage.readDirect(packFile, &fileZoom, 1) != 1 || fileZoom != zoom)
    {
        ESP_LOGE(TAG, "Zoom mismatch in packed file for %s", path);
        closePack();
        return false;
    }

    uint32_t hdrRest[4];
    if (storage.readDirect(packFile, (uint8_t*)hdrRest, 16) != 16)
    {
        ESP_LOGE(TAG, "Failed to read NPK2 header for %s", path);
        closePack();
        return false;
    }

    uint16_t colorCount;
    if (storage.readDirect(packFile, (uint8_t*)&colorCount, 2) != 2)
    {
        ESP_LOGE(TAG, "Failed to read palette size for %s", path);
        closePack();
        return false;
    }

    tilesWide   = hdrRest[0];
    tilesHigh   = hdrRest[1];
    minX        = hdrRest[2];
    minY        = hdrRest[3];

    if (storage.readDirect(packFile, (uint8_t*)&indexCount, sizeof(indexCount)) != sizeof(indexCount))
    {
        ESP_LOGE(TAG, "Failed to read sparse index count for %s", path);
        closePack();
        return false;
    }

    bitmapBytes = ((uint64_t)tilesWide * tilesHigh + 7) / 8;
    uint32_t rankCount = (bitmapBytes + NAV_RANK_STRIDE_BYTES - 1) / NAV_RANK_STRIDE_BYTES;

    indexBase   = NAV_PACK_HDR_SIZE + NAV_SPARSE_COUNT_SIZE;
    rankBase    = indexBase + bitmapBytes;
    entriesBase = rankBase + rankCount * sizeof(uint32_t);
    uint32_t paletteOff = entriesBase + (uint64_t)indexCount * sizeof(IndexEntry);

    if (colorCount > 0)
    {
        colorPalette = static_cast<uint16_t*>(heap_caps_malloc(colorCount * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
        if (!colorPalette)
            colorPalette = static_cast<uint16_t*>(heap_caps_malloc(colorCount * sizeof(uint16_t), MALLOC_CAP_INTERNAL));
        if (!colorPalette || storage.seekAndReadDirect(packFile, paletteOff, (uint8_t*)colorPalette, colorCount * sizeof(uint16_t)) != colorCount * sizeof(uint16_t))
        {
            ESP_LOGE(TAG, "Failed to load color palette for %s", path);
            closePack();
            return false;
        }
        paletteCount = colorCount;
    }

    allocateIndexCache();

    currentZoom = zoom;

    return true;
}

/**
 * @brief Close the currently open packed container.
 */
void NavReader::closePack()
{
    if (packFile)
    {
        storage.close(packFile);
        packFile = nullptr;
    }

    if (colorPalette)
    {
        heap_caps_free(colorPalette);
        colorPalette = nullptr;
    }
    paletteCount = 0;

    freeIndexCache();

    currentZoom = 0;
    tilesWide   = 0;
    tilesHigh   = 0;
    minX        = 0;
    minY        = 0;

    indexBase   = 0;
    rankBase    = 0;
    entriesBase = 0;
    bitmapBytes = 0;
    indexCount  = 0;
}

/**
 * @brief Find a tile in the open pack via the sparse bitmap + rank index.
 *
 * Lookup: read the 64-byte coverage bitmap block, test the cell bit, read the
 * cumulative popcount for the block, then read the compact 8-byte entry. Empty
 * cells resolve with a single small read instead of a full 512 KB index band.
 *
 * @param tileX  Absolute tile X coordinate.
 * @param tileY  Absolute tile Y coordinate.
 * @param offset Output: byte offset of tile data in the file.
 * @param size   Output: byte size of tile data.
 * @return True if tile exists and is non-empty.
 */
bool NavReader::findTileInPack(uint32_t tileX, uint32_t tileY, uint32_t& offset, uint32_t& size)
{
    if (!packFile || tilesWide == 0 || tilesHigh == 0 || bitmapBytes == 0)
        return false;

    if (tileX < minX || tileY < minY)
        return false;

    uint32_t xOff = tileX - minX;
    uint32_t yOff = tileY - minY;

    if (xOff >= tilesWide || yOff >= tilesHigh)
        return false;

    uint32_t flat      = yOff * tilesWide + xOff;
    uint32_t bitByte   = flat >> 3;
    uint32_t blockIdx  = bitByte >> 6;
    uint32_t blockStart = blockIdx << 6;

    uint8_t blockTmp[NAV_RANK_STRIDE_BYTES];
    uint8_t* block = blockTmp;
    bool blkLoaded = false;
    uint32_t blockLen = NAV_RANK_STRIDE_BYTES;
    if (blockLen > bitmapBytes - blockStart)
        blockLen = bitmapBytes - blockStart;

    if (idxBlkMem)
    {
        bool blkHit = false;
        for (uint8_t i = 0; i < IDX_CACHE_BLKS && !blkHit; i++)
        {
            if (idxBlkTag[i] == blockIdx)
            {
                block = idxBlkMem + i * NAV_RANK_STRIDE_BYTES;
                blkLoaded = true;
                blkHit = true;
            }
        }
        if (!blkHit)
        {
            const uint8_t slot = idxBlkFresh;
            idxBlkFresh = (idxBlkFresh + 1) % IDX_CACHE_BLKS;
            block = idxBlkMem + slot * NAV_RANK_STRIDE_BYTES;
            idxBlkTag[slot] = blockIdx;
        }
    }

    if (!blkLoaded)
    {
        const bool ok = storage.seekAndReadDirect(packFile, indexBase + blockStart, block, blockLen) == blockLen;
        if (!ok)
            return false;
    }

    if (!(block[bitByte - blockStart] & (uint8_t)(1u << (flat & 7))))
        return false;

    uint32_t rank = 0;
    bool rankFound = false;
    if (idxRankMem)
    {
        for (uint8_t i = 0; i < IDX_CACHE_BLKS && !rankFound; i++)
        {
            if (idxRankTag[i] == blockIdx)
            {
                rank = idxRankMem[i];
                rankFound = true;
            }
        }
        if (!rankFound)
        {
            const uint8_t slot = idxRankFresh;
            idxRankFresh = (idxRankFresh + 1) % IDX_CACHE_BLKS;
            idxRankTag[slot] = blockIdx;
            const bool ok = storage.seekAndReadDirect(packFile, rankBase + blockIdx * sizeof(uint32_t), (uint8_t*)&idxRankMem[slot], sizeof(uint32_t)) == sizeof(uint32_t);
            if (!ok)
                return false;
            rank = idxRankMem[slot];
        }
    }
    else
    {
        const bool ok = storage.seekAndReadDirect(packFile, rankBase + blockIdx * sizeof(uint32_t), (uint8_t*)&rank, sizeof(uint32_t)) == sizeof(uint32_t);
        if (!ok)
            return false;
    }

    uint32_t within = 0;
    uint32_t end = bitByte - blockStart;
    for (uint32_t i = 0; i < end; ++i)
        within += (uint32_t)__builtin_popcount(block[i]);
    uint8_t cellByte = block[bitByte - blockStart];
    within += (uint32_t)__builtin_popcount(cellByte & ((1u << (flat & 7)) - 1));

    IndexEntry entry;
    const uint64_t entryKey = (uint64_t)rank + within;
    bool entryFound = false;
    if (idxEntryMem)
    {
        for (uint8_t i = 0; i < IDX_CACHE_ENTRIES; i++)
        {
            if (idxEntryTag[i] == entryKey)
            {
                entry = idxEntryMem[i];
                entryFound = true;
                break;
            }
        }
    }
    if (!entryFound)
    {
        const uint32_t entryOff = entriesBase + entryKey * sizeof(IndexEntry);
        const bool ok = storage.seekAndReadDirect(packFile, entryOff, (uint8_t*)&entry, sizeof(IndexEntry)) == sizeof(IndexEntry);
        if (!ok)
            return false;
        if (idxEntryMem)
        {
            const uint8_t slot = (uint8_t)(entryKey % IDX_CACHE_ENTRIES);
            idxEntryMem[slot] = entry;
            idxEntryTag[slot] = entryKey;
        }
    }

    offset = entry.offset;
    size   = entry.size;
    return true;
}
