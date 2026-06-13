/**
 * @file nav_reader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Binary NAV file reader and tile container manager
 * @version 0.2.9
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

NavReader::IndexEntry* NavReader::bandBuffer   = nullptr;
uint32_t               NavReader::bandStartRow = 0;
uint32_t               NavReader::bandRows     = 0;

uint16_t* NavReader::colorPalette = nullptr;
uint16_t  NavReader::paletteCount = 0;

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

    uint16_t colorCount;
    if (storage.read(packFile, (uint8_t*)&colorCount, 2) != 2)
    {
        ESP_LOGE(TAG, "Failed to read palette size for %s", path);
        closePack();
        return false;
    }

    tilesWide   = hdrRest[0];
    tilesHigh   = hdrRest[1];
    minX        = hdrRest[2];
    minY        = hdrRest[3];

    if (colorCount > 0)
    {
        uint32_t paletteOff = NAV_PACK_HDR_SIZE + (uint32_t)tilesWide * tilesHigh * sizeof(IndexEntry);
        colorPalette = static_cast<uint16_t*>(heap_caps_malloc(colorCount * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
        if (!colorPalette)
            colorPalette = static_cast<uint16_t*>(heap_caps_malloc(colorCount * sizeof(uint16_t), MALLOC_CAP_INTERNAL));
        if (!colorPalette || storage.seekAndRead(packFile, paletteOff, (uint8_t*)colorPalette, colorCount * sizeof(uint16_t)) != colorCount * sizeof(uint16_t))
        {
            ESP_LOGE(TAG, "Failed to load color palette for %s", path);
            closePack();
            return false;
        }
        paletteCount = colorCount;
    }

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

    freeBand();

    if (colorPalette)
    {
        heap_caps_free(colorPalette);
        colorPalette = nullptr;
    }
    paletteCount = 0;

    currentZoom = 0;
    tilesWide   = 0;
    tilesHigh   = 0;
    minX        = 0;
    minY        = 0;
}

/**
 * @brief Release the PSRAM index band buffer.
 */
void NavReader::freeBand()
{
    if (bandBuffer)
    {
        heap_caps_free(bandBuffer);
        bandBuffer = nullptr;
    }

    bandStartRow = 0;
    bandRows     = 0;
}

/**
 * @brief Load a horizontal band of index rows into PSRAM, centered on yOff.
 *
 * @param yOff Row offset (tileY - minY) to be covered by the band.
 * @return True if the band buffer covers yOff after the call.
 */
bool NavReader::loadBand(uint32_t yOff)
{
    uint32_t rowBytes = tilesWide * sizeof(IndexEntry);
    if (rowBytes == 0)
        return false;

    uint32_t maxRows = NAV_INDEX_BAND_BYTES / rowBytes;
    if (maxRows == 0)
        maxRows = 1;
    if (maxRows > tilesHigh)
        maxRows = tilesHigh;

    if (!bandBuffer)
    {
        bandBuffer = static_cast<IndexEntry*>(heap_caps_malloc(maxRows * rowBytes, MALLOC_CAP_SPIRAM));
        if (!bandBuffer)
            return false;
    }

    uint32_t startRow;
    if (yOff < maxRows / 2)
        startRow = 0;
    else
        startRow = yOff - maxRows / 2;

    if (startRow + maxRows > tilesHigh)
        startRow = tilesHigh - maxRows;

    uint32_t entryPos = NAV_PACK_HDR_SIZE + startRow * tilesWide * sizeof(IndexEntry);
    if (storage.seekAndRead(packFile, entryPos, (uint8_t*)bandBuffer, maxRows * rowBytes) != maxRows * rowBytes)
    {
        freeBand();
        return false;
    }

    bandStartRow = startRow;
    bandRows     = maxRows;
    return true;
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

    bool inBand = bandBuffer && yOff >= bandStartRow && yOff < bandStartRow + bandRows;
    if (!inBand)
        inBand = loadBand(yOff) && yOff >= bandStartRow && yOff < bandStartRow + bandRows;

    IndexEntry entry;
    if (inBand)
    {
        entry = bandBuffer[(yOff - bandStartRow) * tilesWide + xOff];
    }
    else
    {
        uint32_t entryPos = NAV_PACK_HDR_SIZE + (yOff * tilesWide + xOff) * sizeof(IndexEntry);
        if (storage.seek(packFile, entryPos, SEEK_SET) != 0)
            return false;
        if (storage.read(packFile, (uint8_t*)&entry, sizeof(IndexEntry)) != sizeof(IndexEntry))
            return false;
    }

    if (entry.size == 0)
        return false;

    offset = entry.offset;
    size   = entry.size;
    return true;
}
