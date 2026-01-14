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

/**
 * @brief Constructs NavReader with default state.
 */
NavReader::NavReader()
    : file_(nullptr)
    , bytesRead_(0)
    , headerValid_(false)
{
    memset(&header_, 0, sizeof(header_));
}

/**
 * @brief Destructor - ensures file resources are released.
 */
NavReader::~NavReader()
{
    close();
}

/**
 * @brief Opens a NAV tile file and reads its header.
 * @param path File path to the .nav tile.
 * @return true if opened and header validated successfully, false otherwise.
 */
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

/**
 * @brief Closes the current NAV tile file and resets state.
 */
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

/**
 * @brief Reads and validates the NAV tile header.
 * @return true if header is valid, false if magic bytes mismatch or read error.
 */
bool NavReader::readHeader()
{
    // Read entire header (Magic 4, Count 2, Bbox 16 = 22 bytes)
    uint8_t h[22];
    if (fread(h, 1, 22, file_) != 22)
        return false;
    bytesRead_ += 22;

    if (memcmp(h, NAV_MAGIC, 4) != 0)
    {
        ESP_LOGE(TAG, "Invalid magic bytes");
        return false;
    }

    // Little-endian parsing
    header_.featureCount = h[4] | (h[5] << 8);
    
    // Bbox parsing (4-byte signed integers)
    memcpy(&header_.bbox.minLon, &h[6], 4);
    memcpy(&header_.bbox.minLat, &h[10], 4);
    memcpy(&header_.bbox.maxLon, &h[14], 4);
    memcpy(&header_.bbox.maxLat, &h[18], 4);

    headerValid_ = true;
    return true;
}

/**
 * @brief Reads features from the opened tile, filtered by zoom level and viewport.
 * @param features Output vector to store read features.
 * @param maxZoom  Maximum zoom level to include features for.
 * @param viewport Optional viewport bounding box for culling.
 * @return Number of features successfully read and filtered.
 */
size_t NavReader::readAllFeatures(std::vector<NavFeature>& features, uint8_t maxZoom, const NavBbox* viewport)
{
    if (!file_ || !headerValid_)
        return 0;

    features.reserve(header_.featureCount);
    size_t count = 0;
    size_t skippedZoom = 0;

    for (uint16_t i = 0; i < header_.featureCount; i++)
    {
        // 1. Read Feature Header (7 bytes): Type(1), Color(2), Zoom(1), Width(1), Count(2)
        uint8_t hBuf[7];
        if (fread(hBuf, 1, 7, file_) != 7) break;
        bytesRead_ += 7;

        uint8_t geomType = hBuf[0];
        // Little-endian parsing
        uint16_t color = hBuf[1] | (hBuf[2] << 8);
        uint8_t zoomPriority = hBuf[3];
        uint8_t width = hBuf[4];
        uint16_t coordCount = hBuf[5] | (hBuf[6] << 8);

        // 3. Zoom Filtering
        uint8_t minZoom = zoomPriority >> 4;
        if (minZoom > maxZoom)
        {
            // Skip feature data: coords (8 bytes each) + rings
            long skipBytes = (long)coordCount * 8;
            if (geomType == 3) // Polygon
            {
                uint8_t ringCount = 0;
                if (fread(&ringCount, 1, 1, file_) == 1)
                {
                    bytesRead_++;
                    skipBytes += (ringCount * 2); 
                }
            }
            fseek(file_, skipBytes, SEEK_CUR);
            bytesRead_ += skipBytes;
            skippedZoom++;
            continue;
        }

        NavFeature feature;
        feature.geomType = static_cast<NavGeomType>(geomType);
        feature.properties.colorRgb565 = color;
        feature.properties.zoomPriority = zoomPriority;
        feature.properties.width = width;
        
        // 4. Bulk Read Coordinates
        feature.coords.resize(coordCount);
        if (coordCount > 0)
        {
            size_t bytesToRead = coordCount * 8; // 2 * int32
            if (fread(feature.coords.data(), 1, bytesToRead, file_) != bytesToRead) break;
            bytesRead_ += bytesToRead;
        }

        // 5. Read Polygon Rings
        if (feature.geomType == NavGeomType::Polygon)
        {
            uint8_t ringCount = 0;
            if (fread(&ringCount, 1, 1, file_) == 1)
            {
                bytesRead_++;
                if (ringCount > 0)
                {
                    feature.ringEnds.resize(ringCount);
                    size_t ringBytes = ringCount * 2;
                    if (fread(feature.ringEnds.data(), 1, ringBytes, file_) != ringBytes) break;
                    bytesRead_ += ringBytes;
                }
            }
        }

        features.push_back(std::move(feature));
        count++;
    }

    return count;
}
