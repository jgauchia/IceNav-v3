/**
 * @file nav_reader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Binary NAV file reader and tile container manager
 * @version 0.2.8
 * @date 2026-06
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "storage.hpp"
#include "mapVars.h"

extern Storage storage;
static const char* TAG = "NavReader";

FILE*    NavReader::packFile    = nullptr;
uint8_t  NavReader::currentZoom = 0;
uint32_t NavReader::tilesWide   = 0;
uint32_t NavReader::tilesHigh   = 0;
uint32_t NavReader::minX        = 0;
uint32_t NavReader::minY        = 0;

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

    uint32_t hdrRest[4];
    if (storage.read(packFile, (uint8_t*)hdrRest, 16) != 16)
    {
        ESP_LOGE(TAG, "Failed to read NPK2 header for %s", path);
        closePack();
        return false;
    }

    tilesWide   = hdrRest[0];
    tilesHigh   = hdrRest[1];
    minX        = hdrRest[2];
    minY        = hdrRest[3];
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

    currentZoom = 0;
    tilesWide   = 0;
    tilesHigh   = 0;
    minX        = 0;
    minY        = 0;
}

/**
 * @brief Find a tile in the open pack using O(1) flat 2D index lookup.
 *
 * @param tileX  Absolute tile X coordinate.
 * @param tileY  Absolute tile Y coordinate.
 * @param offset Output: byte offset of tile data in the file.
 * @param size   Output: byte size of tile data.
 * @return True if tile exists and is non-empty.
 */
bool NavReader::findTileInPack(uint32_t tileX, uint32_t tileY, uint32_t& offset, uint32_t& size)
{
    if (!packFile || tilesWide == 0 || tilesHigh == 0)
        return false;

    if (tileX < minX || tileY < minY)
        return false;

    uint32_t xOff = tileX - minX;
    uint32_t yOff = tileY - minY;

    if (xOff >= tilesWide || yOff >= tilesHigh)
        return false;

    uint32_t flatIdx  = yOff * tilesWide + xOff;
    uint32_t entryPos = 21u + flatIdx * 8u;

    if (storage.seek(packFile, entryPos, SEEK_SET) != 0)
        return false;

    IndexEntry entry;
    if (storage.read(packFile, (uint8_t*)&entry, sizeof(IndexEntry)) != sizeof(IndexEntry))
        return false;

    if (entry.size == 0)
        return false;

    offset = entry.offset;
    size   = entry.size;
    return true;
}
