/**
 * @file nav_reader.cpp
 * @brief NAV tile reader implementation for ESP32
 * @version 1.0.0
 * @date 2025-01
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "storage.hpp"

extern Storage storage;

static const char* TAG = "NavReader";

/**
 * @brief Load entire file to memory and read features using zero-copy pointers.
 * 
 * @param path File path to the .nav tile.
 * @param features Output vector to store pointers to data within tileBuffer.
 * @param maxZoom Maximum zoom level to filter features.
 * @param tileBuffer Pointer to the PSRAM buffer (will be reallocated if too small).
 * @param bufferSize Current size of the tileBuffer.
 * @return Number of features loaded and filtered.
 */
size_t NavReader::readAllFeaturesMemory(const char* path, std::vector<NavFeature>& features, uint8_t maxZoom, uint8_t*& tileBuffer, size_t& bufferSize)
{
    FILE* f = storage.open(path, "rb");
    if (!f)
        return 0;

    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize == 0)
    {
        storage.close(f);
        return 0;
    }

    if (fileSize > bufferSize)
    {
        if (tileBuffer)
            heap_caps_free(tileBuffer);
        bufferSize = fileSize + 4096; // Some extra room
        tileBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
    }

    if (!tileBuffer)
    {
        storage.close(f);
        return 0;
    }

    if (storage.read(f, tileBuffer, fileSize) != fileSize)
    {
        storage.close(f);
        return 0;
    }
    storage.close(f);

    // Parsing from memory
    uint8_t* p = tileBuffer;
    NavTileHeader* tileHeader = (NavTileHeader*)p;
    
    if (memcmp(tileHeader->magic, NAV_MAGIC, 4) != 0)
        return 0;
    
    uint16_t featureCount = tileHeader->featureCount;
    features.reserve(featureCount);
    
    p += sizeof(NavTileHeader); // 22 bytes
    size_t count = 0;

    for (uint16_t i = 0; i < featureCount; i++)
    {
        NavFeatureHeader* featHeader = (NavFeatureHeader*)p;
        p += sizeof(NavFeatureHeader); // 12 bytes

        uint8_t minZoom = featHeader->zoomPriority >> 4;
        if (minZoom > maxZoom)
        {
            p += (featHeader->coordCount * 4); // Skip coordinates
            if (featHeader->geomType == 3) // Polygon
            {
                uint8_t ringCount = *p++;
                p += (ringCount * 2); // Skip ring ends
            }
            continue;
        }

        NavFeature feature;
        feature.geomType = static_cast<NavGeomType>(featHeader->geomType);
        feature.properties.colorRgb565 = featHeader->colorRgb565;
        feature.properties.zoomPriority = featHeader->zoomPriority;
        feature.properties.width = featHeader->widthPixels;
        feature.objBbox = { featHeader->bbox[0], featHeader->bbox[1], featHeader->bbox[2], featHeader->bbox[3] };
        feature.coordCount = featHeader->coordCount;
        
        // Zero-copy coordinate pointer
        feature.coords = (NavCoord*)p;
        p += (feature.coordCount * 4);

        if (feature.geomType == NavGeomType::Polygon)
        {
            feature.ringCount = *p++;
            if (feature.ringCount > 0)
            {
                feature.ringEnds = (uint16_t*)p;
                p += (feature.ringCount * 2);
            }
        }

        features.push_back(std::move(feature));
        count++;
    }

    return count;
}
