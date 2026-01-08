/**
 * @file fgb_reader.hpp
 * @brief FlatGeobuf reader for ESP32 with R-Tree spatial queries
 * @version 0.1.0
 * @date 2025-01
 *
 * Reads FlatGeobuf files from SD card using R-Tree spatial index
 * for efficient bbox queries without loading entire file.
 *
 * FlatGeobuf format specification:
 * https://github.com/flatgeobuf/flatgeobuf/blob/master/SPEC.md
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

/**
 * @brief FlatGeobuf magic bytes
 */
static constexpr uint8_t FGB_MAGIC[] = {0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x01};
static constexpr size_t FGB_MAGIC_SIZE = 8;

/**
 * @brief Geometry types in FlatGeobuf
 */
enum class FgbGeometryType : uint8_t
{
    Unknown = 0,
    Point = 1,
    LineString = 2,
    Polygon = 3,
    MultiPoint = 4,
    MultiLineString = 5,
    MultiPolygon = 6,
    GeometryCollection = 7
};

/**
 * @brief Column/property types in FlatGeobuf
 */
enum class FgbColumnType : uint8_t
{
    Byte = 0,
    UByte = 1,
    Bool = 2,
    Short = 3,
    UShort = 4,
    Int = 5,
    UInt = 6,
    Long = 7,
    ULong = 8,
    Float = 9,
    Double = 10,
    String = 11,
    Json = 12,
    DateTime = 13,
    Binary = 14
};

/**
 * @brief Bounding box structure
 */
struct FgbBbox
{
    double minX;
    double minY;
    double maxX;
    double maxY;

    bool intersects(const FgbBbox& other) const
    {
        return !(other.minX > maxX || other.maxX < minX ||
                 other.minY > maxY || other.maxY < minY);
    }
};

/**
 * @brief Column definition from header
 */
struct FgbColumn
{
    char name[32];
    FgbColumnType type;
};

/**
 * @brief FlatGeobuf header information
 */
struct FgbHeader
{
    char name[64];
    FgbBbox envelope;
    FgbGeometryType geometryType;
    bool hasZ;
    bool hasM;
    bool hasT;
    bool hasTM;
    uint16_t indexNodeSize;     // R-Tree node size (0 = no index)
    uint32_t featuresCount;
    std::vector<FgbColumn> columns;
};

/**
 * @brief R-Tree node for spatial indexing
 */
struct FgbRTreeNode
{
    FgbBbox bbox;
    uint64_t offset;    // Feature offset in file (for leaf nodes)
};

/**
 * @brief Single coordinate point
 */
struct FgbCoord
{
    double x;   // longitude
    double y;   // latitude
};

/**
 * @brief Feature properties relevant for rendering
 */
struct FgbProperties
{
    uint16_t colorRgb565;
    uint8_t zoomPriority;  // Packed: high nibble = min_zoom, low nibble = priority/7

    // Helper methods to unpack
    uint8_t getMinZoom() const { return zoomPriority >> 4; }
    uint8_t getPriority() const { return (zoomPriority & 0x0F) * 7; }
};

/**
 * @brief Parsed feature ready for rendering
 */
struct FgbFeature
{
    FgbGeometryType geometryType;
    std::vector<FgbCoord> coordinates;
    std::vector<uint32_t> ringEnds;     // For polygons: end index of each ring
    FgbProperties properties;
};

/**
 * @brief FlatGeobuf reader class
 *
 * Reads FGB files from SD card with spatial indexing support.
 * Designed for memory-constrained ESP32 environment.
 */
class FgbReader
{
public:
    FgbReader();
    ~FgbReader();

    /**
     * @brief Open FGB file and read header/metadata
     * @param path Path to FGB file on SD card
     * @return true if file opened and metadata read successfully
     */
    bool open(const char* path);

    /**
     * @brief Close current file handle (keeps metadata cached)
     */
    void close();

    /**
     * @brief Reopen file for reading (uses cached path)
     * @return true if file reopened successfully
     */
    bool reopen();

    /**
     * @brief Check if file handle is open
     */
    bool isOpen() const { return file_ != nullptr; }

    /**
     * @brief Check if metadata was loaded (can reopen)
     */
    bool isInitialized() const { return rtreeLoaded_; }

    /**
     * @brief Get header information
     */
    const FgbHeader& getHeader() const { return header_; }

    /**
     * @brief Query features within bounding box
     *
     * Uses R-Tree index for efficient spatial query.
     * Returns feature offsets that intersect the bbox.
     *
     * @param bbox Bounding box to query
     * @param maxFeatures Maximum features to return (memory limit)
     * @return Vector of feature offsets
     */
    std::vector<uint64_t> queryBbox(const FgbBbox& bbox, size_t maxFeatures = 1000);

    /**
     * @brief Read a single feature at offset
     * @param offset File offset of feature
     * @param feature Output feature structure
     * @return true if feature read successfully
     */
    bool readFeature(uint64_t offset, FgbFeature& feature);

    /**
     * @brief Read multiple features sequentially (more efficient for SD)
     * @param offsets Vector of feature offsets (will be sorted)
     * @param features Output vector of features
     * @param maxZoom Filter features by min_zoom <= maxZoom
     * @return Number of features read successfully
     */
    size_t readFeaturesSequential(std::vector<uint64_t>& offsets, std::vector<FgbFeature>& features, uint8_t maxZoom);

    /**
     * @brief Get total bytes read (for statistics)
     */
    size_t getBytesRead() const { return bytesRead_; }

    /**
     * @brief Reset bytes read counter
     */
    void resetBytesRead() { bytesRead_ = 0; }

private:
    FILE* file_;
    char filePath_[256];    // Cached path for reopen
    FgbHeader header_;
    size_t bytesRead_;

    // File structure offsets
    uint64_t headerOffset_;
    uint64_t indexOffset_;
    uint64_t featuresOffset_;
    uint32_t indexSize_;        // Size of R-Tree index in bytes

    // R-Tree fully loaded to PSRAM
    std::vector<uint64_t> levelBounds_;  // Node count at each level boundary
    uint32_t numLevels_;
    uint64_t numNodes_;
    bool rtreeLoaded_;
    uint8_t* rtreeData_;      // Full R-Tree index in PSRAM
    size_t rtreeDataSize_;

    // Internal methods
    bool readMagic();
    bool readHeader();
    bool readRTreeIndex();
    bool parseHeaderFlatBuffer(const uint8_t* data, size_t size);
    bool parseFeatureFlatBuffer(const uint8_t* data, size_t size, FgbFeature& feature);

    // FlatBuffer helpers
    uint32_t readU32LE(const uint8_t* data);
    uint64_t readU64LE(const uint8_t* data);
    double readF64LE(const uint8_t* data);

    // Column index cache (for fast property lookup)
    int colIndexColorRgb565_;
    int colIndexZoomPriority_;
};

