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
        // 1. Read Feature Header (5 bytes)
        uint8_t geomType = readU8();
        uint16_t color = readU16();
        uint8_t zoomPriority = readU8();
        uint8_t width = readU8();
        
        // 2. Read Coordinate Count (2 bytes)
        uint16_t coordCount = readU16();

        // 3. Zoom Filtering
        uint8_t minZoom = zoomPriority >> 4;
        if (minZoom > maxZoom)
        {
            // Skip feature data: coords (4+4)*N + rings
            long skipBytes = coordCount * 8;
            if (geomType == 3) // Polygon
            {
                uint8_t ringCount = readU8();
                skipBytes += 1 + (ringCount * 2); // 1 byte count + 2 bytes per ringEnd
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
        
        feature.coords.resize(coordCount);
        
        // 4. Viewport Culling Optimization
        // Read first coordinate to check if feature is vaguely near viewport
        if (coordCount > 0)
        {
            feature.coords[0].lon = readI32();
            feature.coords[0].lat = readI32();

            // Simple point-in-bbox check for the first point (expanded slightly for safety)
            if (viewport)
            {
                const int32_t margin = 5000; // ~50 meters margin
                if (feature.coords[0].lon < viewport->minLon - margin ||
                    feature.coords[0].lon > viewport->maxLon + margin ||
                    feature.coords[0].lat < viewport->minLat - margin ||
                    feature.coords[0].lat > viewport->maxLat + margin)
                {
                    // If first point is far away and it's a small feature (Point/Line), maybe skip?
                    // NOTE: For long lines/polygons, this is risky. 
                    // For now, we only skip reading if zoom failed. 
                    // To safely cull by bbox without reading all points, we need a bbox in the file header.
                    // Proceed to read the rest.
                }
            }

            // Read remaining coordinates
            for (uint16_t k = 1; k < coordCount; k++)
            {
                feature.coords[k].lon = readI32();
                feature.coords[k].lat = readI32();
            }
        }

        // 5. Read Polygon Rings
        if (feature.geomType == NavGeomType::Polygon)
        {
            uint8_t ringCount = readU8();
            feature.ringEnds.resize(ringCount);
            for (uint8_t k = 0; k < ringCount; k++)
            {
                feature.ringEnds[k] = readU16();
            }
        }

        features.push_back(std::move(feature));
        count++;
    }

    return count;
}

/**
 * @brief Helper to read an 8-bit unsigned integer from file.
 * @return The value read.
 */
uint8_t NavReader::readU8()
{
    uint8_t value = 0;
    if (fread(&value, 1, 1, file_) == 1)
        bytesRead_ += 1;
    return value;
}

/**
 * @brief Helper to read a 16-bit unsigned integer (little-endian) from file.
 * @return The value read.
 */
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

/**
 * @brief Helper to read a 32-bit signed integer (little-endian) from file.
 * @return The value read.
 */
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
