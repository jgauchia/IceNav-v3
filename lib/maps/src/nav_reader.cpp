#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"
#include "storage.hpp"
extern Storage storage;
static const char* TAG = "NavReader";
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
        bufferSize = fileSize + 4096;
        tileBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!tileBuffer)
            tileBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT);
    }
    if (!tileBuffer)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffer for map tile");
        storage.close(f);
        return 0;
    }
    if (storage.read(f, tileBuffer, fileSize) != fileSize)
    {
        storage.close(f);
        return 0;
    }
    storage.close(f);
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
        if (minZoom > maxZoom)
        {
            p += featHeader->payloadSize;
            continue;
        }
        NavFeature feature;
        feature.geomType = static_cast<NavGeomType>(featHeader->geomType);
        feature.properties.colorRgb565 = featHeader->colorRgb565;
        feature.properties.zoomPriority = featHeader->zoomPriority;
        feature.properties.width = featHeader->widthPixels;
        feature.objBbox = { featHeader->bbox[0], featHeader->bbox[1], featHeader->bbox[2], featHeader->bbox[3] };
        feature.coordCount = featHeader->coordCount;
        feature.payloadSize = featHeader->payloadSize;
        feature.payloadPtr = p;
        p += featHeader->payloadSize;
        features.push_back(std::move(feature));
        count++;
    }
    return count;
}