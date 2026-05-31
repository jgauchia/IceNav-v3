/**
 * @file nav_reader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Binary NAV file reader and tile container manager
 * @version 0.2.7
 * @date 2026-05
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "storage.hpp"
#include "mapVars.h"

extern Storage storage;
static const char* TAG = "NavReader";

FILE* NavReader::packFile = nullptr;
uint8_t NavReader::currentZoom = 0;
uint32_t NavReader::tileCount = 0;
uint32_t NavReader::indexOff = 0;
NavReader::IndexEntry* NavReader::ramIndex = nullptr;
uint32_t NavReader::ramIndexCount = 0;

/**
 * @brief Open a packed tile container for the given zoom level.
 *s
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
    if (storage.read(packFile, (uint8_t*)magic, 4) != 4 || memcmp(magic, "NPK2", 4) != 0)
    {
        ESP_LOGE(TAG, "Invalid packed magic for %s", path);
        closePack();
        return false;
    }

    uint8_t fileZoom;
    if (storage.read(packFile, &fileZoom, 1) != 1 || fileZoom != zoom)
    {
        ESP_LOGE(TAG, "Zoom mismatch in packed file for %s", path);
        closePack();
        return false;
    }

    // NPK2 Header: 
    // tile_count(4), index_off(4), reserved[4](16) = 24 bytes
    uint32_t headerData[2]; // count, indexOff
    if (storage.read(packFile, (uint8_t*)headerData, 8) != 8)
    {
        ESP_LOGE(TAG, "Failed to read NPK2 header for %s", path);
        closePack();
        return false;
    }

    tileCount = headerData[0];
    indexOff = headerData[1];
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

    if (ramIndex)
    {
        heap_caps_free(ramIndex);
        ramIndex = nullptr;
    }

    currentZoom = 0;
    tileCount = 0;
    indexOff = 0;
    ramIndexCount = 0;
}

/**
 * @brief Search for a tile in the open pack using global Hilbert binary search.
 * @param tileX Tile X coordinate.
 * @param tileY Tile Y coordinate.
 * @param offset Output offset.
 * @param size Output size.
 * @return True if found.
 */
void NavReader::prefetchIndexRange(const uint32_t* tileXs, const uint32_t* tileYs, uint8_t count, uint8_t zoom)
{
    if (!packFile || tileCount == 0 || count == 0)
        return;

    if (ramIndex)
    {
        heap_caps_free(ramIndex);
        ramIndex = nullptr;
        ramIndexCount = 0;
    }

    uint64_t hMin = UINT64_MAX;
    uint64_t hMax = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        uint64_t h = xyToHilbert(tileXs[i], tileYs[i], zoom);
        if (h < hMin)
            hMin = h;
        if (h > hMax)
            hMax = h;
    }

    // Binary search for first entry >= hMin
    int32_t lo = 0;
    int32_t hi = (int32_t)tileCount - 1;
    int32_t idxLow = (int32_t)tileCount;
    while (lo <= hi)
    {
        int32_t mid = lo + (hi - lo) / 2;
        storage.seek(packFile, indexOff + (uint32_t)mid * 16, SEEK_SET);
        uint64_t h;
        if (storage.read(packFile, (uint8_t*)&h, 8) != 8)
            return;
        if (h < hMin)
            lo = mid + 1;
        else
        {
            idxLow = mid;
            hi = mid - 1;
        }
    }

    // Binary search for last entry <= hMax
    lo = 0;
    hi = (int32_t)tileCount - 1;
    int32_t idxHigh = -1;
    while (lo <= hi)
    {
        int32_t mid = lo + (hi - lo) / 2;
        storage.seek(packFile, indexOff + (uint32_t)mid * 16, SEEK_SET);
        uint64_t h;
        if (storage.read(packFile, (uint8_t*)&h, 8) != 8)
            return;
        if (h > hMax)
            hi = mid - 1;
        else
        {
            idxHigh = mid;
            lo = mid + 1;
        }
    }

    if (idxLow > idxHigh || idxHigh < 0)
    {
        ESP_LOGW(TAG, "prefetch z%u: no entries in Hilbert range", zoom);
        return;
    }

    uint32_t rangeCount = (uint32_t)(idxHigh - idxLow + 1);
    size_t rangeBytes = rangeCount * sizeof(IndexEntry);

    ramIndex = (IndexEntry*)heap_caps_malloc(rangeBytes, MALLOC_CAP_SPIRAM);
    if (!ramIndex)
    {
        ESP_LOGE(TAG, "prefetch z%u: alloc failed (%u KB)", zoom, (unsigned)(rangeBytes / 1024));
        return;
    }

    storage.seek(packFile, indexOff + (uint32_t)idxLow * 16, SEEK_SET);
    if (storage.read(packFile, (uint8_t*)ramIndex, rangeBytes) != rangeBytes)
    {
        heap_caps_free(ramIndex);
        ramIndex = nullptr;
        ESP_LOGE(TAG, "prefetch z%u: read failed", zoom);
        return;
    }

    ramIndexCount = rangeCount;
}

bool NavReader::findTileInPack(uint32_t tileX, uint32_t tileY, uint32_t& offset, uint32_t& size)
{
    if (tileCount == 0)
        return false;

    uint64_t targetH = xyToHilbert(tileX, tileY, currentZoom);

    if (ramIndex && ramIndexCount > 0)
    {
        int32_t low = 0;
        int32_t high = (int32_t)ramIndexCount - 1;
        while (low <= high)
        {
            int32_t mid = low + (high - low) / 2;
            uint64_t entryH = ramIndex[mid].hilbert;
            if (entryH < targetH)
                low = mid + 1;
            else if (entryH > targetH)
                high = mid - 1;
            else
            {
                offset = ramIndex[mid].offset;
                size   = ramIndex[mid].size;
                return true;
            }
        }
        return false;
    }

    if (!packFile)
        return false;

    int32_t low = 0;
    int32_t high = (int32_t)tileCount - 1;

    while (low <= high)
    {
        int32_t mid = low + (high - low) / 2;
        storage.seek(packFile, indexOff + (mid * 16), SEEK_SET);

        uint64_t entryH;
        if (storage.read(packFile, (uint8_t*)&entryH, 8) != 8)
            return false;

        if (entryH < targetH)
            low = mid + 1;
        else if (entryH > targetH)
            high = mid - 1;
        else
        {
            if (storage.read(packFile, (uint8_t*)&offset, 4) != 4 ||
                storage.read(packFile, (uint8_t*)&size, 4) != 4)
                return false;
            return true;
        }
    }

    return false;
}
