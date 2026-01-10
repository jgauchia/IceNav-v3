/**
 * @file nav_reader.cpp
 * @brief NAV tile reader implementation for ESP32
 * @version 1.0.0
 * @date 2025-01
 */

#include "nav_reader.hpp"
#include <cstring>
#include "esp_log.h"

static const char* TAG = "NavReader";

// File buffer for improved SD card performance
static constexpr size_t FILE_BUFFER_SIZE = 4096;
static char fileBuffer_[FILE_BUFFER_SIZE];

NavReader::NavReader()
    : file_(nullptr)
    , bytesRead_(0)
    , headerValid_(false)
{
    memset(&header_, 0, sizeof(header_));
}

NavReader::~NavReader()
{
    close();
}

bool NavReader::open(const char* path)
{
    close();

    file_ = fopen(path, "rb");
    if (!file_)
    {
        ESP_LOGD(TAG, "Failed to open: %s", path);
        return false;
    }

    // Set file buffer for better SD performance
    setvbuf(file_, fileBuffer_, _IOFBF, FILE_BUFFER_SIZE);

    if (!readHeader())
    {
        ESP_LOGE(TAG, "Invalid NAV header: %s", path);
        close();
        return false;
    }

    ESP_LOGD(TAG, "Opened NAV: %s, features: %u", path, header_.featureCount);
    return true;
}

void NavReader::close()
{
    if (file_)
    {
        fclose(file_);
        file_ = nullptr;
    }
    headerValid_ = false;
    bytesRead_ = 0;
}

bool NavReader::readHeader()
{
    // Read magic (4 bytes)
    uint8_t magic[4];
    if (fread(magic, 1, 4, file_) != 4)
        return false;
    bytesRead_ += 4;

    if (memcmp(magic, NAV_MAGIC, 4) != 0)
    {
        ESP_LOGE(TAG, "Invalid magic bytes");
        return false;
    }

    // Read feature count (2 bytes)
    header_.featureCount = readU16();

    // Read bbox (16 bytes)
    header_.bbox.minLon = readI32();
    header_.bbox.minLat = readI32();
    header_.bbox.maxLon = readI32();
    header_.bbox.maxLat = readI32();

    headerValid_ = true;
    return true;
}

size_t NavReader::readAllFeatures(std::vector<NavFeature>& features, uint8_t maxZoom)
{
    if (!file_ || !headerValid_)
        return 0;

    features.reserve(header_.featureCount);
    size_t count = 0;

    for (uint16_t i = 0; i < header_.featureCount; i++)
    {
        NavFeature feature;
        if (readFeature(feature))
        {
            // Filter by zoom
            if (feature.properties.getMinZoom() <= maxZoom)
            {
                features.push_back(std::move(feature));
                count++;
            }
        }
    }

    return count;
}

bool NavReader::readFeature(NavFeature& feature)
{
    // Read geometry type (1 byte)
    uint8_t geomType = readU8();
    if (geomType < 1 || geomType > 3)
        return false;
    feature.geomType = static_cast<NavGeomType>(geomType);

    // Read color (2 bytes)
    feature.properties.colorRgb565 = readU16();

    // Read zoom_priority (1 byte)
    feature.properties.zoomPriority = readU8();

    // Read width (1 byte)
    feature.properties.width = readU8();

    // Read coordinate count (2 bytes)
    uint16_t coordCount = readU16();
    if (coordCount == 0 || coordCount > 10000)  // Sanity check
        return false;

    // Read coordinates
    feature.coords.resize(coordCount);
    for (uint16_t i = 0; i < coordCount; i++)
    {
        feature.coords[i].lon = readI32();
        feature.coords[i].lat = readI32();
    }

    // For polygons, read ring info
    if (feature.geomType == NavGeomType::Polygon)
    {
        uint8_t ringCount = readU8();
        feature.ringEnds.resize(ringCount);
        for (uint8_t i = 0; i < ringCount; i++)
        {
            feature.ringEnds[i] = readU16();
        }
    }

    return true;
}

uint8_t NavReader::readU8()
{
    uint8_t value = 0;
    if (fread(&value, 1, 1, file_) == 1)
        bytesRead_ += 1;
    return value;
}

uint16_t NavReader::readU16()
{
    uint8_t buf[2];
    if (fread(buf, 1, 2, file_) == 2)
    {
        bytesRead_ += 2;
        return buf[0] | (buf[1] << 8);
    }
    return 0;
}

int32_t NavReader::readI32()
{
    uint8_t buf[4];
    if (fread(buf, 1, 4, file_) == 4)
    {
        bytesRead_ += 4;
        return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    }
    return 0;
}
