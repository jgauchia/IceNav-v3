/**
 * @file nav_reader.cpp
 * @brief NAV tile reader implementation for ESP32
 * @version 1.0.1
 * @date 2026-02
 * 
 * Updated for NAV v2 Format:
 * - Header 13 bytes (payload_size)
 * - VarInt + ZigZag + Delta coordinates
 * - Polygons rings at end of payload
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "storage.hpp"

extern Storage storage;
static const char* TAG = "NavReader";

// --- Helper Functions for Delta/VarInt Decoding ---

static int32_t readVarInt(uint8_t*& p)
{
    int32_t result = 0;
    int shift = 0;
    while (true)
    {
        uint8_t byte = *p++;
        result |= (byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

static int32_t decodeZigZag(int32_t n)
{
    return (n >> 1) ^ -(n & 1);
}

// --------------------------------------------------

/**
 * @brief Load entire file to memory and read features using efficient parsing.
 * @param path File path to the .nav tile.
 * @param features Output vector to store feature objects.
 * @param maxZoom Maximum zoom level to filter features.
 * @param tileBuffer Reference to the PSRAM buffer.
 * @param bufferSize Current size of the tileBuffer.
 * @return Number of features loaded.
 */
size_t NavReader::readAllFeaturesMemory(const char* path, std::vector<NavFeature>& features, uint8_t maxZoom, uint8_t*& tileBuffer, size_t& bufferSize)
{
    FILE* f = storage.open(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fileSize == 0)
    {
        storage.close(f);
        return 0;
    }
    
    // Allocate or resize buffer if needed
    if (fileSize > bufferSize)
    {
        if (tileBuffer) heap_caps_free(tileBuffer);
        bufferSize = fileSize + 4096;
        tileBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
    }
    if (!tileBuffer)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffer for map tile");
        storage.close(f);
        return 0;
    }
    
    // Read entire file into memory
    if (storage.read(f, tileBuffer, fileSize) != fileSize)
    {
        storage.close(f);
        return 0;
    }
    storage.close(f);

    // Start Parsing
    uint8_t* p = tileBuffer;
    NavTileHeader* tileHeader = (NavTileHeader*)p;
    if (memcmp(tileHeader->magic, NAV_MAGIC, 4) != 0)
    {
        ESP_LOGE(TAG, "Invalid NAV magic");
        return 0;
    }

    uint16_t featureCount = tileHeader->featureCount;
    features.reserve(featureCount);
    p += sizeof(NavTileHeader);
    
    size_t count = 0;
    for (uint16_t i = 0; i < featureCount; i++)
    {
        NavFeatureHeader* featHeader = (NavFeatureHeader*)p;
        p += sizeof(NavFeatureHeader);
        
        uint8_t minZoom = featHeader->zoomPriority >> 4;
        
        // 1. Fast Culling: Check Zoom
        if (minZoom > maxZoom)
        {
            // Skip payload
            p += featHeader->payloadSize;
            continue;
        }
        
        // 2. Prepare Feature
        NavFeature feature;
        feature.geomType = static_cast<NavGeomType>(featHeader->geomType);
        feature.properties.colorRgb565 = featHeader->colorRgb565;
        feature.properties.zoomPriority = featHeader->zoomPriority;
        feature.properties.width = featHeader->widthPixels;
        feature.objBbox = { featHeader->bbox[0], featHeader->bbox[1], featHeader->bbox[2], featHeader->bbox[3] };
        feature.coordCount = featHeader->coordCount;
        
        // Keep track of payload start for this feature
        uint8_t* payloadStart = p;
        
        // 3. Decode Coordinates (VarInt + ZigZag + Delta)
        // Reserve memory to avoid reallocations
        feature.coords.reserve(feature.coordCount);
        
        int32_t lastX = 0;
        int32_t lastY = 0;
        
        for(uint16_t k = 0; k < feature.coordCount; k++)
        {
             // Read Delta X
             int32_t dx = decodeZigZag(readVarInt(p));
             // Read Delta Y
             int32_t dy = decodeZigZag(readVarInt(p));
             
             // Accumulate
             lastX += dx;
             lastY += dy;
             
             feature.coords.push_back({(int16_t)lastX, (int16_t)lastY});
        }
        
        // 4. Handle Polygons (Rings)
        if (feature.geomType == NavGeomType::Polygon)
        {
            // Rings are at the end of the payload.
            // Check if we have data left in the payload
            size_t bytesRead = p - payloadStart;
            if (bytesRead < featHeader->payloadSize)
            {
                 // Read Ring Count (assuming 2 bytes LE)
                 uint16_t rCount = p[0] | (p[1] << 8);
                 p += 2;
                 
                 feature.ringCount = rCount;
                 if (rCount > 0)
                 {
                     feature.ringEnds.reserve(rCount);
                     for(int r=0; r<rCount; r++)
                     {
                         uint16_t endIdx = p[0] | (p[1] << 8);
                         p += 2;
                         feature.ringEnds.push_back(endIdx);
                     }
                 }
            }
        }
        
        // 5. Finalize: Jump to exact end of payload (safety against malformed data)
        p = payloadStart + featHeader->payloadSize;

        features.push_back(std::move(feature));
        count++;
    }
    
    return count;
}