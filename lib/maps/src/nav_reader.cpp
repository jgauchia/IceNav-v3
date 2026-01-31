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
    if (memcmp(p, NAV_MAGIC, 4) != 0)
        return 0;
    
    uint16_t featureCount = p[4] | (p[5] << 8);
    features.reserve(featureCount);
    
    p += 22; // Skip header (Magic 4, Count 2, Bbox 16)
    size_t count = 0;

    for (uint16_t i = 0; i < featureCount; i++)
    {
        uint8_t geomType = p[0];
        uint16_t color = p[1] | (p[2] << 8);
        uint8_t zoomPriority = p[3];
        uint8_t width = p[4];
        uint16_t coordCount = p[5] | (p[6] << 8);
        p += 7;

        uint8_t minZoom = zoomPriority >> 4;
        if (minZoom > maxZoom)
        {
            p += (coordCount * 8);
            if (geomType == 3) // Polygon
            {
                uint8_t ringCount = *p++;
                p += (ringCount * 2);
            }
            continue;
        }

        NavFeature feature;
        feature.geomType = static_cast<NavGeomType>(geomType);
        feature.properties.colorRgb565 = color;
        feature.properties.zoomPriority = zoomPriority;
        feature.properties.width = width;
        feature.coordCount = coordCount;
        feature.coords = (NavCoord*)p;
        
        // Calculate Bbox on the fly
        if (coordCount > 0)
        {
            int32_t minLon = INT32_MAX, maxLon = INT32_MIN;
            int32_t minLat = INT32_MAX, maxLat = INT32_MIN;
            NavCoord* c = feature.coords;
            
            for (int k = 0; k < coordCount; k++)
            {
                if (c[k].lon < minLon) minLon = c[k].lon;
                if (c[k].lon > maxLon) maxLon = c[k].lon;
                if (c[k].lat < minLat) minLat = c[k].lat;
                if (c[k].lat > maxLat) maxLat = c[k].lat;
            }
            feature.bbox.minLon = minLon;
            feature.bbox.maxLon = maxLon;
            feature.bbox.minLat = minLat;
            feature.bbox.maxLat = maxLat;
        }
        
        p += (coordCount * 8);

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
