/**
 * @file fgb_reader.cpp
 * @brief FlatGeobuf reader implementation for ESP32
 * @version 0.1.0
 * @date 2025-01
 */

#include "fgb_reader.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "FgbReader";

// File I/O buffer for improved SD card performance
// Larger buffer reduces number of actual SD reads (default is ~128-512 bytes)
static constexpr size_t FILE_BUFFER_SIZE = 4096;
static char fileBuffer_[FILE_BUFFER_SIZE];

// FlatBuffer constants
static constexpr uint32_t FB_SIZE_PREFIX = 4;

FgbReader::FgbReader()
    : file_(nullptr)
    , bytesRead_(0)
    , headerOffset_(0)
    , indexOffset_(0)
    , featuresOffset_(0)
    , indexSize_(0)
    , numLevels_(0)
    , numNodes_(0)
    , rtreeLoaded_(false)
    , rtreeData_(nullptr)
    , rtreeDataSize_(0)
    , colIndexColorRgb565_(-1)
    , colIndexMinZoom_(-1)
    , colIndexPriority_(-1)
{
    memset(&header_, 0, sizeof(header_));
    memset(filePath_, 0, sizeof(filePath_));
}

FgbReader::~FgbReader()
{
    close();
    if (rtreeData_)
    {
        free(rtreeData_);
        rtreeData_ = nullptr;
    }
}

bool FgbReader::open(const char* path)
{
    close();

    // Save path for reopen
    strncpy(filePath_, path, sizeof(filePath_) - 1);
    filePath_[sizeof(filePath_) - 1] = '\0';

    file_ = fopen(path, "rb");
    if (!file_)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return false;
    }

    // Set larger file buffer for better SD card performance
    setvbuf(file_, fileBuffer_, _IOFBF, FILE_BUFFER_SIZE);

    // Read and validate magic bytes
    if (!readMagic())
    {
        ESP_LOGE(TAG, "Invalid FGB magic bytes for: %s", path);
        close();
        return false;
    }

    // Read header
    if (!readHeader())
    {
        ESP_LOGE(TAG, "Failed to read header");
        close();
        return false;
    }

    ESP_LOGI(TAG, "Opened FGB: %s, features: %lu, geom: %d, indexNodeSize: %u",
             header_.name, (unsigned long)header_.featuresCount,
             (int)header_.geometryType, header_.indexNodeSize);

    // Read R-Tree index if present
    if (header_.indexNodeSize > 0)
    {
        if (!readRTreeIndex())
        {
            ESP_LOGW(TAG, "Failed to read R-Tree index, will do linear scan");
        }
    }
    else
    {
        ESP_LOGW(TAG, "No spatial index in file (indexNodeSize=0)");
    }

    // Close file after reading metadata - will reopen on demand
    fclose(file_);
    file_ = nullptr;
    ESP_LOGD(TAG, "Closed file after metadata read, will reopen on demand");

    return true;
}

bool FgbReader::reopen()
{
    if (file_)
        return true;  // Already open

    if (filePath_[0] == '\0')
    {
        ESP_LOGE(TAG, "Cannot reopen: no path saved");
        return false;
    }

    // Retry with delays if SD card is busy
    for (int retry = 0; retry < 3; retry++)
    {
        if (retry > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(100 * retry));  // 100ms, 200ms delays
        }

        file_ = fopen(filePath_, "rb");
        if (file_)
        {
            // Set larger file buffer for better SD card performance
            setvbuf(file_, fileBuffer_, _IOFBF, FILE_BUFFER_SIZE);
            return true;
        }
    }

    ESP_LOGE(TAG, "Failed to reopen file after retries: %s", filePath_);
    return false;
}

void FgbReader::close()
{
    if (file_)
    {
        fclose(file_);
        file_ = nullptr;
    }
    // Note: We keep metadata (header_, levelBounds_, etc.) for reopen
    bytesRead_ = 0;
}

bool FgbReader::readMagic()
{
    uint8_t magic[FGB_MAGIC_SIZE] = {0};
    size_t read = fread(magic, 1, FGB_MAGIC_SIZE, file_);
    bytesRead_ += read;

    if (read != FGB_MAGIC_SIZE)
        return false;

    return memcmp(magic, FGB_MAGIC, FGB_MAGIC_SIZE) == 0;
}

bool FgbReader::readHeader()
{
    headerOffset_ = FGB_MAGIC_SIZE;

    // Read header size (4 bytes, little-endian)
    uint8_t sizeBytes[4];
    if (fread(sizeBytes, 1, 4, file_) != 4)
        return false;
    bytesRead_ += 4;

    uint32_t headerSize = readU32LE(sizeBytes);
    if (headerSize == 0 || headerSize > 1024 * 1024) // Sanity check: max 1MB header
        return false;

    // Read header FlatBuffer
    std::vector<uint8_t> headerData(headerSize);
    if (fread(headerData.data(), 1, headerSize, file_) != headerSize)
        return false;
    bytesRead_ += headerSize;

    // Parse header
    if (!parseHeaderFlatBuffer(headerData.data(), headerSize))
        return false;

    // Set index offset (R-Tree size calculated in readRTreeIndex)
    indexOffset_ = FGB_MAGIC_SIZE + 4 + headerSize;

    ESP_LOGI(TAG, "Header parsed: indexOffset=%llu", (unsigned long long)indexOffset_);

    return true;
}

bool FgbReader::parseHeaderFlatBuffer(const uint8_t* data, size_t size)
{
    if (size < 4)
        return false;

    // FlatBuffer root table offset
    uint32_t rootOffset = readU32LE(data);
    if (rootOffset >= size)
        return false;

    const uint8_t* root = data + rootOffset;

    // Read vtable offset (signed, relative to root)
    int32_t vtableOffset = static_cast<int32_t>(readU32LE(root));
    const uint8_t* vtable = root - vtableOffset;

    // vtable: [vtable_size:u16] [table_size:u16] [field_offsets:u16...]
    if (vtable < data || vtable + 4 > data + size)
        return false;

    uint16_t vtableSize = vtable[0] | (vtable[1] << 8);
    // uint16_t tableSize = vtable[2] | (vtable[3] << 8);

    // Helper to get field offset from vtable
    auto getFieldOffset = [&](int fieldIndex) -> uint16_t {
        size_t offsetPos = 4 + fieldIndex * 2;
        if (offsetPos + 2 > vtableSize)
            return 0;
        return vtable[offsetPos] | (vtable[offsetPos + 1] << 8);
    };

    // Field indices in FGB Header (from flatgeobuf schema):
    // 0: name (string)
    // 1: envelope (double[4])
    // 2: geometry_type (ubyte)
    // 3: has_z (bool)
    // 4: has_m (bool)
    // 5: has_t (bool)
    // 6: has_tm (bool)
    // 7: columns (vector of Column)
    // 8: features_count (ulong)
    // 9: index_node_size (ushort)
    // ... more fields

    // Name (field 0)
    uint16_t nameOffset = getFieldOffset(0);
    if (nameOffset > 0)
    {
        uint32_t strOffset = readU32LE(root + nameOffset);
        const uint8_t* strPtr = root + nameOffset + strOffset;
        uint32_t strLen = readU32LE(strPtr);
        if (strLen < sizeof(header_.name))
        {
            memcpy(header_.name, strPtr + 4, strLen);
            header_.name[strLen] = '\0';
        }
    }

    // Envelope (field 1) - array of 4 doubles
    uint16_t envOffset = getFieldOffset(1);
    if (envOffset > 0)
    {
        uint32_t vecOffset = readU32LE(root + envOffset);
        const uint8_t* vecPtr = root + envOffset + vecOffset;
        uint32_t vecLen = readU32LE(vecPtr);
        if (vecLen >= 4)
        {
            const uint8_t* doubles = vecPtr + 4;
            header_.envelope.minX = readF64LE(doubles);
            header_.envelope.minY = readF64LE(doubles + 8);
            header_.envelope.maxX = readF64LE(doubles + 16);
            header_.envelope.maxY = readF64LE(doubles + 24);
        }
    }

    // Geometry type (field 2)
    uint16_t geomOffset = getFieldOffset(2);
    if (geomOffset > 0)
    {
        header_.geometryType = static_cast<FgbGeometryType>(root[geomOffset]);
    }

    // has_z, has_m, has_t, has_tm (fields 3-6) - booleans
    uint16_t hasZOffset = getFieldOffset(3);
    if (hasZOffset > 0) header_.hasZ = root[hasZOffset] != 0;

    uint16_t hasMOffset = getFieldOffset(4);
    if (hasMOffset > 0) header_.hasM = root[hasMOffset] != 0;

    // Columns (field 7) - vector of Column tables
    uint16_t colsOffset = getFieldOffset(7);
    if (colsOffset > 0)
    {
        uint32_t vecOffset = readU32LE(root + colsOffset);
        const uint8_t* vecPtr = root + colsOffset + vecOffset;
        uint32_t numCols = readU32LE(vecPtr);

        const uint8_t* colOffsets = vecPtr + 4;
        header_.columns.reserve(numCols);

        for (uint32_t i = 0; i < numCols && i < 20; i++) // Max 20 columns
        {
            uint32_t colOffset = readU32LE(colOffsets + i * 4);
            const uint8_t* colTable = colOffsets + i * 4 + colOffset;

            // Parse Column table (simplified)
            // Column has: name (string), type (ubyte)
            FgbColumn col;
            memset(&col, 0, sizeof(col));

            int32_t colVtOffset = static_cast<int32_t>(readU32LE(colTable));
            const uint8_t* colVt = colTable - colVtOffset;
            uint16_t colVtSize = colVt[0] | (colVt[1] << 8);

            // Field 0: name
            if (colVtSize >= 6)
            {
                uint16_t nameOff = colVt[4] | (colVt[5] << 8);
                if (nameOff > 0)
                {
                    uint32_t strOff = readU32LE(colTable + nameOff);
                    const uint8_t* strP = colTable + nameOff + strOff;
                    uint32_t strL = readU32LE(strP);
                    if (strL < sizeof(col.name))
                    {
                        memcpy(col.name, strP + 4, strL);
                        col.name[strL] = '\0';
                    }
                }
            }

            // Field 1: type
            if (colVtSize >= 8)
            {
                uint16_t typeOff = colVt[6] | (colVt[7] << 8);
                if (typeOff > 0)
                {
                    col.type = static_cast<FgbColumnType>(colTable[typeOff]);
                }
            }

            header_.columns.push_back(col);
            ESP_LOGD(TAG, "Column[%d]: '%s' type=%d", i, col.name, (int)col.type);

            // Cache column indices for fast property lookup
            if (strcmp(col.name, "color_rgb565") == 0) colIndexColorRgb565_ = i;
            else if (strcmp(col.name, "min_zoom") == 0) colIndexMinZoom_ = i;
            else if (strcmp(col.name, "priority") == 0) colIndexPriority_ = i;
        }
    }

    // Features count (field 8)
    uint16_t countOffset = getFieldOffset(8);
    if (countOffset > 0)
    {
        header_.featuresCount = readU64LE(root + countOffset);
    }

    // Index node size (field 9) - default is 16 if not specified
    uint16_t nodeOffset = getFieldOffset(9);
    if (nodeOffset > 0)
    {
        header_.indexNodeSize = root[nodeOffset] | (root[nodeOffset + 1] << 8);
    }
    else
    {
        header_.indexNodeSize = 16; // FlatGeobuf default
    }

    ESP_LOGI(TAG, "Columns: colorRgb565=%d, minZoom=%d, priority=%d",
             colIndexColorRgb565_, colIndexMinZoom_, colIndexPriority_);

    return true;
}

bool FgbReader::readRTreeIndex()
{
    if (header_.indexNodeSize == 0 || header_.featuresCount == 0)
        return false;

    // Calculate R-Tree structure using packed Hilbert R-Tree formula
    // Following official FlatGeobuf generateLevelBounds
    uint64_t numItems = header_.featuresCount;
    uint16_t nodeSize = header_.indexNodeSize;

    // Step 1: Calculate node counts per level (leaves to root)
    std::vector<uint64_t> levelNumNodes;
    uint64_t n = numItems;
    uint64_t numNodes = n;
    levelNumNodes.push_back(n);  // Level 0 = leaves

    while (n > 1) {
        n = (n + nodeSize - 1) / nodeSize;
        numNodes += n;
        levelNumNodes.push_back(n);
    }

    // Step 2: Calculate level offsets from end to start
    std::vector<uint64_t> levelOffsets;
    n = numNodes;
    for (auto size : levelNumNodes) {
        levelOffsets.push_back(n -= size);
    }

    // Step 3: Build levelBounds as (start, end) pairs
    // levelBounds_[k] = end of level k (for compatibility with my loop)
    levelBounds_.clear();
    for (size_t i = 0; i < levelNumNodes.size(); i++) {
        levelBounds_.push_back(levelOffsets[i] + levelNumNodes[i]);
    }

    numLevels_ = levelBounds_.size();
    numNodes_ = numNodes;

    // Each node is 40 bytes
    indexSize_ = numNodes * 40;
    featuresOffset_ = indexOffset_ + indexSize_;

    ESP_LOGI(TAG, "R-Tree: %llu nodes, %u levels, %u bytes (streaming mode)",
             (unsigned long long)numNodes_, numLevels_, indexSize_);

    rtreeLoaded_ = true;
    return true;
}

std::vector<uint64_t> FgbReader::queryBbox(const FgbBbox& bbox, size_t maxFeatures)
{
    std::vector<uint64_t> results;

    if (!rtreeLoaded_)
    {
        ESP_LOGW(TAG, "R-Tree not loaded");
        return results;
    }

    // Reopen file if closed
    if (!file_ && !reopen())
    {
        ESP_LOGE(TAG, "Failed to reopen file for query");
        return results;
    }

    // Initial delay after opening file
    vTaskDelay(pdMS_TO_TICKS(50));

    uint64_t numItems = header_.featuresCount;
    uint16_t nodeSize = header_.indexNodeSize;

    // FlatGeobuf R-Tree structure:
    // - Nodes 0 to numNodes-numItems-1: branch nodes (ROOT at position 0!)
    // - Nodes numNodes-numItems to numNodes-1: leaf nodes
    uint64_t leafNodesOffset = numNodes_ - numItems;

    static constexpr size_t NODE_SIZE = 40;
    std::vector<uint8_t> nodeBuffer(nodeSize * NODE_SIZE);

    // Use map for queue: nodeIndex -> level (like official implementation)
    std::map<uint64_t, uint64_t> searchQueue;
    searchQueue[0] = numLevels_ - 1;  // Start at root (node 0)

    size_t readCount = 0;
    while (!searchQueue.empty())
    {
        auto it = searchQueue.begin();
        uint64_t nodeIndex = it->first;
        uint64_t level = it->second;
        searchQueue.erase(it);

        bool isLeafNode = nodeIndex >= leafNodesOffset;

        // Calculate end of range
        uint64_t levelEnd = levelBounds_[level];
        uint64_t end = std::min(nodeIndex + nodeSize, levelEnd);
        uint64_t count = end - nodeIndex;

        // Read nodes from file
        fseek(file_, indexOffset_ + nodeIndex * NODE_SIZE, SEEK_SET);
        size_t bytesToRead = count * NODE_SIZE;
        if (fread(nodeBuffer.data(), 1, bytesToRead, file_) != bytesToRead)
            break;
        bytesRead_ += bytesToRead;

        // Delay every few reads to let SD card recover
        if (++readCount % 5 == 0)
            vTaskDelay(pdMS_TO_TICKS(50));

        // Check each node
        for (uint64_t i = 0; i < count; i++)
        {
            const uint8_t* node = nodeBuffer.data() + i * NODE_SIZE;
            uint64_t pos = nodeIndex + i;

            FgbBbox nodeBbox;
            nodeBbox.minX = readF64LE(node);
            nodeBbox.minY = readF64LE(node + 8);
            nodeBbox.maxX = readF64LE(node + 16);
            nodeBbox.maxY = readF64LE(node + 24);
            uint64_t nodeOffset = readU64LE(node + 32);

            if (!nodeBbox.intersects(bbox))
                continue;

            if (isLeafNode || pos >= leafNodesOffset)
            {
                // Leaf node - offset is feature byte offset
                results.push_back(nodeOffset);
                if (results.size() >= maxFeatures)
                    return results;
            }
            else
            {
                // Branch node - queue children (nodeOffset points to first child)
                if (level > 0)
                {
                    searchQueue[nodeOffset] = level - 1;
                }
            }
        }
    }

    // Delay before next operation
    vTaskDelay(pdMS_TO_TICKS(50));

    return results;
}

bool FgbReader::readFeature(uint64_t offset, FgbFeature& feature)
{
    // Reopen file if closed
    if (!file_ && !reopen())
        return false;

    // Seek to feature position
    if (fseek(file_, featuresOffset_ + offset, SEEK_SET) != 0)
        return false;

    // Read feature size
    uint8_t sizeBytes[4];
    if (fread(sizeBytes, 1, 4, file_) != 4)
        return false;
    bytesRead_ += 4;

    uint32_t featureSize = readU32LE(sizeBytes);
    if (featureSize == 0 || featureSize > 1024 * 1024) // Max 1MB feature
        return false;

    // Read feature FlatBuffer
    std::vector<uint8_t> featureData(featureSize);
    if (fread(featureData.data(), 1, featureSize, file_) != featureSize)
        return false;
    bytesRead_ += featureSize;

    // Yield after larger reads to avoid SD timeouts
    if (featureSize > 1024)
        vTaskDelay(pdMS_TO_TICKS(1));

    return parseFeatureFlatBuffer(featureData.data(), featureSize, feature);
}

size_t FgbReader::readFeaturesSequential(std::vector<uint64_t>& offsets, std::vector<FgbFeature>& features, uint8_t maxZoom)
{
    if (offsets.empty())
        return 0;

    // Reopen file if closed
    if (!file_ && !reopen())
        return 0;

    // Sort offsets for sequential reading
    std::sort(offsets.begin(), offsets.end());

    // Calculate range to read
    uint64_t minOffset = offsets.front();
    uint64_t maxOffset = offsets.back();

    // Estimate max feature size (most features are < 4KB)
    static constexpr size_t AVG_FEATURE_SIZE = 2048;
    static constexpr size_t MAX_BUFFER_SIZE = 2 * 1024 * 1024;  // 2MB max buffer (PSRAM available)

    size_t estimatedSize = (maxOffset - minOffset) + AVG_FEATURE_SIZE;
    ESP_LOGI(TAG, "Sequential read range: %llu-%llu (%zu bytes needed)",
             (unsigned long long)minOffset, (unsigned long long)maxOffset, estimatedSize);

    if (estimatedSize > MAX_BUFFER_SIZE)
    {
        // Too large, fall back to individual reads with yields
        ESP_LOGW(TAG, "Feature range too large (%zu bytes > %zu), using individual reads",
                 estimatedSize, MAX_BUFFER_SIZE);
        features.reserve(offsets.size());
        size_t count = 0;
        for (size_t i = 0; i < offsets.size(); i++)
        {
            FgbFeature feature;
            if (readFeature(offsets[i], feature))
            {
                if (feature.properties.minZoom <= maxZoom)
                {
                    features.push_back(std::move(feature));
                    count++;
                }
            }
            if ((i + 1) % 20 == 0)
                vTaskDelay(pdMS_TO_TICKS(50));
        }
        return count;
    }

    // Allocate buffer in PSRAM if available
    uint8_t* buffer = (uint8_t*)heap_caps_malloc(estimatedSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer)
    {
        // Try regular RAM
        buffer = (uint8_t*)malloc(estimatedSize);
        if (!buffer)
        {
            ESP_LOGE(TAG, "Failed to allocate %zu bytes for sequential read", estimatedSize);
            return 0;
        }
        ESP_LOGI(TAG, "Using regular RAM for %zu byte buffer", estimatedSize);
    }
    else
    {
        ESP_LOGI(TAG, "Using PSRAM for %zu byte buffer", estimatedSize);
    }

    // Seek to start and read entire range
    fseek(file_, featuresOffset_ + minOffset, SEEK_SET);
    size_t bytesRead = fread(buffer, 1, estimatedSize, file_);
    bytesRead_ += bytesRead;

    if (bytesRead == 0)
    {
        free(buffer);
        ESP_LOGE(TAG, "Failed to read feature data");
        return 0;
    }

    ESP_LOGI(TAG, "Sequential read: %zu bytes in one operation", bytesRead);

    // Parse features from buffer
    features.reserve(offsets.size());
    size_t count = 0;

    for (uint64_t offset : offsets)
    {
        uint64_t bufferOffset = offset - minOffset;
        if (bufferOffset + 4 > bytesRead)
            continue;

        // Read feature size
        uint32_t featureSize = readU32LE(buffer + bufferOffset);
        if (featureSize == 0 || featureSize > 1024 * 1024)
            continue;

        if (bufferOffset + 4 + featureSize > bytesRead)
            continue;

        // Parse feature
        FgbFeature feature;
        if (parseFeatureFlatBuffer(buffer + bufferOffset + 4, featureSize, feature))
        {
            if (feature.properties.minZoom <= maxZoom)
            {
                features.push_back(std::move(feature));
                count++;
            }
        }
    }

    free(buffer);
    return count;
}

bool FgbReader::parseFeatureFlatBuffer(const uint8_t* data, size_t size, FgbFeature& feature)
{
    if (size < 4)
        return false;

    // Feature FlatBuffer structure:
    // - geometry (Geometry table)
    // - properties (ubyte vector)
    // - columns (vector of Column, usually inherited from header)

    uint32_t rootOffset = readU32LE(data);
    if (rootOffset >= size)
        return false;

    const uint8_t* root = data + rootOffset;
    int32_t vtableOffset = static_cast<int32_t>(readU32LE(root));
    const uint8_t* vtable = root - vtableOffset;

    uint16_t vtableSize = vtable[0] | (vtable[1] << 8);

    auto getFieldOffset = [&](int fieldIndex) -> uint16_t {
        size_t offsetPos = 4 + fieldIndex * 2;
        if (offsetPos + 2 > vtableSize)
            return 0;
        return vtable[offsetPos] | (vtable[offsetPos + 1] << 8);
    };

    // Field 0: geometry
    uint16_t geomOffset = getFieldOffset(0);
    if (geomOffset > 0)
    {
        uint32_t geomTableOffset = readU32LE(root + geomOffset);
        const uint8_t* geomTable = root + geomOffset + geomTableOffset;

        // Parse Geometry table
        int32_t geomVtOffset = static_cast<int32_t>(readU32LE(geomTable));
        const uint8_t* geomVt = geomTable - geomVtOffset;
        uint16_t geomVtSize = geomVt[0] | (geomVt[1] << 8);

        // Geometry fields:
        // 0: ends (uint vector) - for rings/parts
        // 1: xy (double vector) - coordinates
        // 2: z (double vector)
        // 3: m (double vector)
        // 4: t (double vector)
        // 5: tm (ulong vector)
        // 6: type (ubyte)

        auto getGeomFieldOffset = [&](int fieldIndex) -> uint16_t {
            size_t offsetPos = 4 + fieldIndex * 2;
            if (offsetPos + 2 > geomVtSize)
                return 0;
            return geomVt[offsetPos] | (geomVt[offsetPos + 1] << 8);
        };

        // Type (field 6)
        uint16_t typeOff = getGeomFieldOffset(6);
        if (typeOff > 0)
        {
            feature.geometryType = static_cast<FgbGeometryType>(geomTable[typeOff]);
        }
        else
        {
            feature.geometryType = header_.geometryType;
        }

        // Ends (field 0) - for polygons
        uint16_t endsOff = getGeomFieldOffset(0);
        if (endsOff > 0)
        {
            uint32_t vecOff = readU32LE(geomTable + endsOff);
            const uint8_t* vecPtr = geomTable + endsOff + vecOff;
            uint32_t numEnds = readU32LE(vecPtr);
            feature.ringEnds.resize(numEnds);
            const uint8_t* endsData = vecPtr + 4;
            for (uint32_t i = 0; i < numEnds; i++)
            {
                feature.ringEnds[i] = readU32LE(endsData + i * 4);
            }
        }

        // XY coordinates (field 1)
        uint16_t xyOff = getGeomFieldOffset(1);
        if (xyOff > 0)
        {
            uint32_t vecOff = readU32LE(geomTable + xyOff);
            const uint8_t* vecPtr = geomTable + xyOff + vecOff;
            uint32_t numDoubles = readU32LE(vecPtr);
            uint32_t numCoords = numDoubles / 2;
            feature.coordinates.resize(numCoords);
            const uint8_t* xyData = vecPtr + 4;
            for (uint32_t i = 0; i < numCoords; i++)
            {
                feature.coordinates[i].x = readF64LE(xyData + i * 16);
                feature.coordinates[i].y = readF64LE(xyData + i * 16 + 8);
            }
        }
    }

    // Field 1: properties (ubyte vector)
    uint16_t propsOffset = getFieldOffset(1);
    if (propsOffset > 0)
    {
        uint32_t vecOff = readU32LE(root + propsOffset);
        const uint8_t* vecPtr = root + propsOffset + vecOff;
        uint32_t propsSize = readU32LE(vecPtr);
        const uint8_t* propsData = vecPtr + 4;

        // Properties are encoded as: [col_index:u16][value...]
        // Value size depends on column type

        size_t propOffset = 0;
        while (propOffset + 2 <= propsSize)
        {
            uint16_t colIdx = propsData[propOffset] | (propsData[propOffset + 1] << 8);
            propOffset += 2;

            if (colIdx >= header_.columns.size())
                break;

            const FgbColumn& col = header_.columns[colIdx];

            // Helper lambda to read integer value based on column type
            auto readIntValue = [&](size_t& offset) -> int64_t {
                int64_t value = 0;
                switch (col.type)
                {
                    case FgbColumnType::Byte:
                    case FgbColumnType::UByte:
                        if (offset < propsSize) value = propsData[offset++];
                        break;
                    case FgbColumnType::Short:
                    case FgbColumnType::UShort:
                        if (offset + 2 <= propsSize) {
                            value = propsData[offset] | (propsData[offset + 1] << 8);
                            offset += 2;
                        }
                        break;
                    case FgbColumnType::Int:
                    case FgbColumnType::UInt:
                        if (offset + 4 <= propsSize) {
                            value = readU32LE(propsData + offset);
                            offset += 4;
                        }
                        break;
                    case FgbColumnType::Long:
                    case FgbColumnType::ULong:
                        if (offset + 8 <= propsSize) {
                            value = static_cast<int64_t>(readU64LE(propsData + offset));
                            offset += 8;
                        }
                        break;
                    default:
                        break;
                }
                return value;
            };

            if ((int)colIdx == colIndexColorRgb565_)
            {
                feature.properties.colorRgb565 = static_cast<uint16_t>(readIntValue(propOffset) & 0xFFFF);
            }
            else if ((int)colIdx == colIndexMinZoom_)
            {
                feature.properties.minZoom = static_cast<uint8_t>(readIntValue(propOffset) & 0xFF);
            }
            else if ((int)colIdx == colIndexPriority_)
            {
                feature.properties.priority = static_cast<uint8_t>(readIntValue(propOffset) & 0xFF);
            }
            else
            {
                // Skip unknown property based on type
                switch (col.type)
                {
                    case FgbColumnType::Byte:
                    case FgbColumnType::UByte:
                    case FgbColumnType::Bool:
                        propOffset += 1;
                        break;
                    case FgbColumnType::Short:
                    case FgbColumnType::UShort:
                        propOffset += 2;
                        break;
                    case FgbColumnType::Int:
                    case FgbColumnType::UInt:
                    case FgbColumnType::Float:
                        propOffset += 4;
                        break;
                    case FgbColumnType::Long:
                    case FgbColumnType::ULong:
                    case FgbColumnType::Double:
                        propOffset += 8;
                        break;
                    case FgbColumnType::String:
                    case FgbColumnType::Json:
                    case FgbColumnType::Binary:
                        if (propOffset + 4 <= propsSize)
                        {
                            uint32_t len = readU32LE(propsData + propOffset);
                            propOffset += 4 + len;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return !feature.coordinates.empty();
}

// Little-endian read helpers
uint32_t FgbReader::readU32LE(const uint8_t* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

uint64_t FgbReader::readU64LE(const uint8_t* data)
{
    return static_cast<uint64_t>(readU32LE(data)) |
           (static_cast<uint64_t>(readU32LE(data + 4)) << 32);
}

double FgbReader::readF64LE(const uint8_t* data)
{
    uint64_t bits = readU64LE(data);
    double result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

