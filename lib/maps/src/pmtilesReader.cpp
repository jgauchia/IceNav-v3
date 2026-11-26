/**
 * @file pmtilesReader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  PMTiles v3 reader for the optional MVT base map backend
 * @version 0.3.3
 * @date 2026-11
 */

#include "pmtilesReader.hpp"

#include <cstring>

#include "esp_heap_caps.h"

// This header comes before any Arduino include, so the project log level would
// drop the open/close lines at compile time.
#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"

#include <lgfx/utility/lgfx_miniz.h>

#include "mapVars.h"
#include "storage.hpp"

extern Storage storage;

static const char* TAG = "PMTILES";

/// Header size of the PMTiles v3 archive.
static constexpr size_t PMTILES_HEADER_SIZE = 127;
/// Largest directory accepted, in entries. Protomaps leaves stay below 15k.
static constexpr uint32_t PMTILES_MAX_ENTRIES = 60000;
/// Directory working buffer: the planet's largest leaf expands to about 101 KB.
static constexpr size_t PMTILES_DIR_BUFFER = 262144;
/// seekAndReadDirect takes a 32-bit offset, so the archive must fit in 2 GiB.
static constexpr uint64_t PMTILES_MAX_ARCHIVE = 0x7FFFFFFFULL;

static constexpr uint8_t PMTILES_COMP_NONE = 0x01;
static constexpr uint8_t PMTILES_COMP_GZIP = 0x02;
static constexpr uint8_t PMTILES_TYPE_MVT = 0x01;
static constexpr uint8_t PMTILES_TYPE_MLT = 0x06;

FILE*                    PmtilesReader::packFile            = nullptr;
bool                     PmtilesReader::opened              = false;
uint8_t                  PmtilesReader::zoomLevel           = 0;
uint64_t                 PmtilesReader::rootOffset          = 0;
uint64_t                 PmtilesReader::rootLength          = 0;
uint64_t                 PmtilesReader::leafOffset          = 0;
uint64_t                 PmtilesReader::tileOffset          = 0;
uint8_t                  PmtilesReader::internalCompression = 0;
uint8_t                  PmtilesReader::tileCompression     = 0;
std::vector<PmtilesReader::Entry> PmtilesReader::rootEntries;
std::vector<PmtilesReader::Entry> PmtilesReader::leafEntries;
uint64_t                 PmtilesReader::leafFirstId         = 0;
bool                     PmtilesReader::leafValid           = false;
uint8_t*                 PmtilesReader::compressedBuffer    = nullptr;
size_t                   PmtilesReader::compressedCapacity  = 0;
uint8_t*                 PmtilesReader::inflateBuffer       = nullptr;
size_t                   PmtilesReader::inflateCapacity     = 0;
const char*              PmtilesReader::errorText           = "";

/// Inflate state kept off the render task stack: miniz carries a 32 KB window
/// inside the decompressor and the task stack is smaller than that.
static lgfx_tinfl_decompressor* inflateState = nullptr;

/**
 * @brief Allocate a buffer preferring PSRAM, falling back to internal RAM.
 *
 * @param bytes Byte count to allocate.
 * @return Pointer to the buffer, or nullptr on failure.
 */
static void* allocPsramFirst(size_t bytes)
{
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!p)
        p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    return p;
}

/**
 * @brief Read an unsigned LEB128 varint from a memory buffer.
 *
 * @details Directory fields are 32-bit in practice for a regional archive: the
 *          largest value is a byte offset below 2 GiB.
 *
 * @param p     Cursor, advanced past the varint.
 * @param end   End of the buffer.
 * @param value Output value.
 * @return True when a complete varint was read.
 */
static bool readVarint(const uint8_t*& p, const uint8_t* end, uint64_t& value)
{
    uint64_t result = 0;
    int shift = 0;
    while (p < end && shift < 64)
    {
        const uint8_t byte = *p++;
        result |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
        {
            value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

/**
 * @brief Decompress a gzip member into a caller buffer.
 *
 * @details The gzip wrapper is stripped here and the raw deflate stream is handed
 *          to the miniz copy bundled with LovyanGFX, which is already built for
 *          both S3 and P4. The decompressor lives in RAM, not on the stack: its
 *          window alone is 32 KB. The CRC and length trailer are not verified:
 *          the SD copy is checked against the archive before it is used.
 *
 * @param src         Compressed bytes, gzip header included.
 * @param srcLength   Length of the compressed bytes.
 * @param dst         Output buffer.
 * @param dstCapacity Capacity of the output buffer.
 * @param dstLength   Output: decompressed length.
 * @return True on success.
 */
bool PmtilesReader::gunzip(const uint8_t* src, size_t srcLength, uint8_t* dst, size_t dstCapacity, size_t& dstLength)
{
    dstLength = 0;
    if (srcLength < 18 || src[0] != 0x1F || src[1] != 0x8B || src[2] != 0x08)
    {
        errorText = "not a gzip member";
        return false;
    }

    const uint8_t flags = src[3];
    size_t pos = 10;
    if (flags & 0x04)
    {
        if (pos + 2 > srcLength)
            return false;
        const size_t extra = (size_t)src[pos] | ((size_t)src[pos + 1] << 8);
        pos += 2 + extra;
    }
    if (flags & 0x08)
    {
        while (pos < srcLength && src[pos] != 0)
            pos++;
        pos++;
    }
    if (flags & 0x10)
    {
        while (pos < srcLength && src[pos] != 0)
            pos++;
        pos++;
    }
    if (flags & 0x02)
        pos += 2;
    if (pos >= srcLength)
    {
        errorText = "truncated gzip header";
        return false;
    }

    if (inflateState == nullptr)
    {
        inflateState = static_cast<lgfx_tinfl_decompressor*>(heap_caps_malloc(sizeof(lgfx_tinfl_decompressor), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (inflateState == nullptr)
            inflateState = static_cast<lgfx_tinfl_decompressor*>(allocPsramFirst(sizeof(lgfx_tinfl_decompressor)));
        if (inflateState == nullptr)
        {
            errorText = "no memory for the inflater";
            return false;
        }
    }

    lgfx_tinfl_init(inflateState);
    size_t inLength = srcLength - pos;
    size_t outLength = dstCapacity;
    const lgfx_tinfl_status status = lgfx_tinfl_decompress(inflateState, src + pos, &inLength, dst, dst, &outLength,
                                                           TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (status != TINFL_STATUS_DONE)
    {
        errorText = (status == TINFL_STATUS_HAS_MORE_OUTPUT) ? "tile does not fit" : "inflate failed";
        return false;
    }
    dstLength = outLength;
    return true;
}

/**
 * @brief Decode an encoded PMTiles directory into entry records.
 *
 * @details Layout is entry count, delta-encoded tile ids, run lengths, lengths
 *          and offsets, all as LEB128 varints. An offset of zero means "directly
 *          after the previous entry".
 *
 * @param offset Byte offset of the directory inside the archive.
 * @param length Byte length of the stored directory.
 * @param out    Output entries.
 * @return True on success.
 */
bool PmtilesReader::readDirectory(uint64_t offset, uint64_t length, std::vector<Entry>& out)
{
    if (length == 0 || length > PMTILES_DIR_BUFFER)
    {
        errorText = "directory too large";
        return false;
    }
    if (!ensureCompressedBuffer((size_t)length))
        return false;

    if (storage.seekAndReadDirect(packFile, (long)(offset), compressedBuffer, (size_t)length) != (size_t)length)
    {
        errorText = "directory read failed";
        return false;
    }

    const uint8_t* data = compressedBuffer;
    size_t dataLength = (size_t)length;
    if (internalCompression == PMTILES_COMP_GZIP)
    {
        if (!ensureInflateBuffer(PMTILES_DIR_BUFFER))
            return false;
        if (!gunzip(compressedBuffer, (size_t)length, inflateBuffer, inflateCapacity, dataLength))
            return false;
        data = inflateBuffer;
    }
    else if (internalCompression != PMTILES_COMP_NONE)
    {
        errorText = "unsupported internal compression";
        return false;
    }

    const uint8_t* p = data;
    const uint8_t* end = data + dataLength;
    uint64_t count = 0;
    if (!readVarint(p, end, count) || count == 0 || count > PMTILES_MAX_ENTRIES)
    {
        errorText = "bad directory count";
        return false;
    }

    out.assign((size_t)count, Entry{0, 0, 0, 0});

    uint64_t id = 0;
    for (size_t i = 0; i < out.size(); i++)
    {
        uint64_t delta = 0;
        if (!readVarint(p, end, delta))
        {
            errorText = "bad tile id";
            return false;
        }
        id += delta;
        out[i].tileId = id;
    }
    for (size_t i = 0; i < out.size(); i++)
    {
        uint64_t run = 0;
        if (!readVarint(p, end, run))
        {
            errorText = "bad run length";
            return false;
        }
        out[i].runLength = (uint32_t)run;
    }
    for (size_t i = 0; i < out.size(); i++)
    {
        uint64_t len = 0;
        if (!readVarint(p, end, len))
        {
            errorText = "bad entry length";
            return false;
        }
        out[i].length = (uint32_t)len;
    }
    uint64_t previousEnd = 0;
    for (size_t i = 0; i < out.size(); i++)
    {
        uint64_t raw = 0;
        if (!readVarint(p, end, raw))
        {
            errorText = "bad entry offset";
            return false;
        }
        const uint64_t entryOffset = (raw == 0) ? previousEnd : raw - 1;
        out[i].offset = entryOffset;
        previousEnd = entryOffset + out[i].length;
    }
    return true;
}

/**
 * @brief Open the archive of one zoom level, named like the native pack.
 *
 * @param zoom Zoom level whose Z{zoom}.pmtiles is opened.
 * @return True when the archive exists, is PMTiles v3 and carries MVT tiles.
 */
bool PmtilesReader::openPack(uint8_t zoom)
{
    // The project boots with the global level at ERROR, so this tag has to raise
    // itself or none of the open and reject lines would ever reach the console.
    esp_log_level_set(TAG, ESP_LOG_INFO);

    if (opened && zoomLevel == zoom)
        return true;

    closePack();

    char path[64];
    snprintf(path, sizeof(path), pmtilesFolder, zoom);
    if (!storage.exists(path))
        return false;

    packFile = storage.open(path, "rb");
    if (!packFile)
    {
        errorText = "open failed";
        return false;
    }

    uint8_t header[PMTILES_HEADER_SIZE];
    if (storage.readDirect(packFile, header, PMTILES_HEADER_SIZE) != PMTILES_HEADER_SIZE)
    {
        errorText = "short header";
        closePack();
        return false;
    }
    if (memcmp(header, "PMTiles", 7) != 0 || header[7] != 3)
    {
        errorText = "not PMTiles v3";
        closePack();
        return false;
    }

    uint64_t field = 0;
    memcpy(&field, header + 8, 8);
    rootOffset = field;
    memcpy(&field, header + 16, 8);
    rootLength = field;
    memcpy(&field, header + 40, 8);
    leafOffset = field;
    memcpy(&field, header + 56, 8);
    tileOffset = field;

    internalCompression = header[97];
    tileCompression = header[98];
    const uint8_t archiveType = header[99];
    const uint8_t archiveMinZoom = header[100];
    const uint8_t archiveMaxZoom = header[101];

    if (archiveType != PMTILES_TYPE_MVT && archiveType != PMTILES_TYPE_MLT)
    {
        errorText = "archive is not MVT";
        closePack();
        return false;
    }
    if (tileCompression != PMTILES_COMP_GZIP && tileCompression != PMTILES_COMP_NONE)
    {
        errorText = "unsupported tile compression";
        closePack();
        return false;
    }
    if (zoom < archiveMinZoom || zoom > archiveMaxZoom)
    {
        errorText = "zoom outside the archive";
        closePack();
        return false;
    }
    if (leafOffset + tileOffset > PMTILES_MAX_ARCHIVE)
    {
        errorText = "archive beyond 2 GiB";
        closePack();
        return false;
    }

    if (!readDirectory(rootOffset, rootLength, rootEntries))
    {
        closePack();
        return false;
    }

    leafValid = false;
    zoomLevel = zoom;
    opened = true;
    ESP_LOGI(TAG, "Open %s: z%u-%u, %u root entries", path, (unsigned)archiveMinZoom,
             (unsigned)archiveMaxZoom, (unsigned)rootEntries.size());
    return true;
}

/**
 * @brief Close the archive and release the directory state.
 */
void PmtilesReader::closePack()
{
    if (packFile)
    {
        storage.close(packFile);
        packFile = nullptr;
    }
    opened = false;
    zoomLevel = 0;
    rootEntries.clear();
    leafEntries.clear();
    rootEntries.shrink_to_fit();
    leafEntries.shrink_to_fit();
    leafValid = false;
    leafFirstId = 0;
    rootOffset = 0;
    rootLength = 0;
    leafOffset = 0;
    tileOffset = 0;
}

/**
 * @brief Report whether an archive is open.
 *
 * @return True when a pack is available for the current zoom.
 */
bool PmtilesReader::isOpen()
{
    return opened && packFile != nullptr;
}

/**
 * @brief Return the reason of the last failure.
 *
 * @return Static string, never null.
 */
const char* PmtilesReader::lastError()
{
    return errorText;
}

/**
 * @brief Convert a slippy tile coordinate to its PMTiles tile id.
 *
 * @details Cumulative Hilbert curve: ids of all previous zooms plus the position
 *          of (x, y) inside the Hilbert curve of this zoom.
 *
 * @param zoom Zoom level.
 * @param tileX Tile column.
 * @param tileY Tile row.
 * @return Tile id used by the directories.
 */
uint64_t PmtilesReader::tileIdFromZxy(uint8_t zoom, uint32_t tileX, uint32_t tileY)
{
    uint64_t accumulated = (zoom == 0) ? 0 : (((uint64_t)1 << (2 * zoom)) - 1) / 3;
    uint64_t distance = 0;
    const uint32_t side = 1u << zoom;
    uint32_t xx = tileX;
    uint32_t yy = tileY;
    for (uint32_t step = side / 2; step > 0; step /= 2)
    {
        const uint32_t rx = (xx & step) ? 1u : 0u;
        const uint32_t ry = (yy & step) ? 1u : 0u;
        distance += (uint64_t)step * (uint64_t)step * (uint64_t)((3u * rx) ^ ry);
        if (ry == 0)
        {
            if (rx == 1)
            {
                xx = side - 1 - xx;
                yy = side - 1 - yy;
            }
            const uint32_t swap = xx;
            xx = yy;
            yy = swap;
        }
    }
    return accumulated + distance;
}

/**
 * @brief Find the last entry whose first tile id is at or before a target.
 *
 * @param entries Directory entries in ascending tile id order.
 * @param tileId  Target tile id.
 * @return Matching entry, or nullptr.
 */
const PmtilesReader::Entry* PmtilesReader::lastAtOrBefore(const std::vector<Entry>& entries, uint64_t tileId)
{
    if (entries.empty() || entries.front().tileId > tileId)
        return nullptr;

    size_t low = 0;
    size_t high = entries.size() - 1;
    while (low < high)
    {
        const size_t mid = (low + high + 1) / 2;
        if (entries[mid].tileId <= tileId)
            low = mid;
        else
            high = mid - 1;
    }
    return &entries[low];
}

/**
 * @brief Resolve a tile to its byte range inside the archive.
 *
 * @details The root directory either points at the tile or at the leaf that
 *          covers it; the leaf is decompressed on first use and kept until
 *          another range is requested.
 *
 * @param tileX  Tile column.
 * @param tileY  Tile row.
 * @param zoom   Zoom level.
 * @param offset Output: absolute offset of the compressed tile.
 * @param length Output: compressed length of the tile.
 * @return True when the tile exists.
 */
bool PmtilesReader::findTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, uint64_t& offset, uint32_t& length)
{
    if (!opened)
    {
        errorText = "archive not open";
        return false;
    }

    const uint64_t tileId = tileIdFromZxy(zoom, tileX, tileY);
    const Entry* entry = lastAtOrBefore(rootEntries, tileId);
    if (!entry)
    {
        errorText = "tile not in archive";
        return false;
    }

    if (entry->runLength == 0)
    {
        if (!leafValid || leafFirstId != entry->tileId)
        {
            if (!readDirectory(leafOffset + entry->offset, entry->length, leafEntries))
                return false;
            leafFirstId = entry->tileId;
            leafValid = true;
        }
        entry = lastAtOrBefore(leafEntries, tileId);
        if (!entry || entry->runLength == 0)
        {
            errorText = "tile not in archive";
            return false;
        }
    }

    if (tileId >= entry->tileId + entry->runLength)
    {
        errorText = "tile not in archive";
        return false;
    }

    offset = tileOffset + entry->offset;
    length = entry->length;
    return true;
}

/**
 * @brief Read a compressed tile and inflate it into the caller buffer.
 *
 * @param offset      Absolute offset of the compressed tile.
 * @param length      Compressed length.
 * @param out         Output buffer.
 * @param outCapacity Capacity of the output buffer.
 * @param outSize     Output: inflated size.
 * @return True on success.
 */
bool PmtilesReader::readTile(uint64_t offset, uint32_t length, uint8_t* out, size_t outCapacity, size_t& outSize)
{
    outSize = 0;
    if (!opened || !out || length == 0)
        return false;
    if (!ensureCompressedBuffer(length))
        return false;
    if (storage.seekAndReadDirect(packFile, (long)offset, compressedBuffer, length) != length)
    {
        errorText = "tile read failed";
        return false;
    }
    if (tileCompression == PMTILES_COMP_NONE)
    {
        if (length > outCapacity)
        {
            errorText = "tile does not fit";
            return false;
        }
        memcpy(out, compressedBuffer, length);
        outSize = length;
        return true;
    }
    return gunzip(compressedBuffer, length, out, outCapacity, outSize);
}

/**
 * @brief Ensure the compressed-tile working buffer can hold a given length.
 *
 * @param bytes Required capacity.
 * @return True when the buffer is ready.
 */
bool PmtilesReader::ensureCompressedBuffer(size_t bytes)
{
    if (compressedBuffer && compressedCapacity >= bytes)
        return true;
    size_t capacity = bytes + (bytes / 4) + 1024;
    uint8_t* resized = static_cast<uint8_t*>(allocPsramFirst(capacity));
    if (!resized)
    {
        errorText = "no memory for the compressed buffer";
        return false;
    }
    if (compressedBuffer)
        heap_caps_free(compressedBuffer);
    compressedBuffer = resized;
    compressedCapacity = capacity;
    return true;
}

/**
 * @brief Ensure the directory inflate buffer is allocated.
 *
 * @param bytes Required capacity.
 * @return True when the buffer is ready.
 */
bool PmtilesReader::ensureInflateBuffer(size_t bytes)
{
    if (inflateBuffer && inflateCapacity >= bytes)
        return true;
    uint8_t* resized = static_cast<uint8_t*>(allocPsramFirst(bytes));
    if (!resized)
    {
        errorText = "no memory for the directory buffer";
        return false;
    }
    if (inflateBuffer)
        heap_caps_free(inflateBuffer);
    inflateBuffer = resized;
    inflateCapacity = bytes;
    return true;
}

/**
 * @brief Release the working buffers and the inflate state.
 *
 * @details The archive stays open: the next directory or tile read allocates
 *          them again on demand.
 */
void PmtilesReader::releaseBuffers()
{
    if (compressedBuffer)
        heap_caps_free(compressedBuffer);
    compressedBuffer = nullptr;
    compressedCapacity = 0;

    if (inflateBuffer)
        heap_caps_free(inflateBuffer);
    inflateBuffer = nullptr;
    inflateCapacity = 0;

    if (inflateState)
        heap_caps_free(inflateState);
    inflateState = nullptr;
}
