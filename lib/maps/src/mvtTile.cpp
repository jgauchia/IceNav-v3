/**
 * @file mvtTile.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  MVT (PMTiles) decoder that feeds the existing vector render pipeline
 * @version 0.3.3
 * @date 2026-11
 */

#include "maps.hpp"

#include <cstring>

#include "esp_heap_caps.h"

#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"

#include "pmtilesReader.hpp"
#include "vectMapReader.hpp"
#include "PsramAllocator.hpp"

static const char* TAG = "MVTTILE";

/// Vertices kept for a single layer feature. Matches Maps::MAX_POLYGON_POINTS,
/// which is private: a longer ring would be refused by the renderer.
static constexpr uint16_t MVT_MAX_POINTS = 8192;
/// Arena block the decoder fills before taking the next one. Six blocks keep the
/// previous ceilings (3 MB on P4, 1,5 MB on S3), but a light pass only pays the
/// block it touches and a dense map grows into the rest: the size stops being a
/// bet on the densest map anybody will load.
#if defined(CONFIG_IDF_TARGET_ESP32P4)
static constexpr size_t MVT_ARENA_BLOCK_BYTES = 512 * 1024;
/// Inflated tile scratch: the densest measured z15 tile is 566 KB.
static constexpr size_t MVT_TILE_BYTES = 1024 * 1024;
#else
static constexpr size_t MVT_ARENA_BLOCK_BYTES = 256 * 1024;
static constexpr size_t MVT_TILE_BYTES = 768 * 1024;
#endif

/// A drawable kind derived from a style rule. None marks a class the firmware knows
/// about on purpose only to keep it off the map.
enum class MvtKind : uint8_t
{
    Fill,
    Line,
    Text,
    None
};

/// One row of the compiled style table: layer and optional class to a drawable.
struct MvtStyleRule
{
    const char* layer;
    const char* cls;
    uint16_t color;
    uint8_t width;
    uint8_t priority;
    uint8_t minZoom;
    MvtKind kind;
};

static constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

/// Compiled style: the rules the firmware draws. Protomaps **basemap v4** layers
/// and classes only, ordered so a specific class wins over its layer default.
/// The layers that the v3 schema had and v4 does not — `natural` and
/// `physical_line` — get no rule: the same ground and water arrive here as
/// `landcover`, `landuse` and `water`, and a rule for a layer no tile carries
/// never matches anything.
///
/// Classes, colours and first zoom follow `features.json` of Tile-Generator, so
/// the vector path draws at least what the raster packs draw, and OSM Carto fills
/// the rows where a basemap class means something other than the OSM tag of the
/// same name (`water`, `basin`, `boundaries`). The priority is the z-order nibble
/// of that file, which is also what the aggressive LOD pass skips while panning.
static const MvtStyleRule MVT_STYLE[] =
{
    {"earth",      "cliff",          rgb565(153, 153, 153),   2,  2, 11, MvtKind::Line},
    {"earth",      "earth",          rgb565(242, 239, 233),   0,  0,  0, MvtKind::Fill},
    {"earth",      nullptr,          rgb565(242, 239, 233),   0,  0,  0, MvtKind::Fill},
    {"landcover",  "farmland",       rgb565(238, 240, 214),   0,  2,  9, MvtKind::Fill},
    {"landcover",  "forest",         rgb565(173, 209, 158),   0,  2,  8, MvtKind::Fill},
    {"landcover",  "glacier",        rgb565(222, 237, 237),   0,  2, 10, MvtKind::Fill},
    {"landcover",  "grassland",      rgb565(206, 236, 177),   0,  5,  9, MvtKind::Fill},
    {"landcover",  "sand",           rgb565(245, 233, 199),   0,  2, 12, MvtKind::Fill},
    {"landcover",  "scrub",          rgb565(201, 215, 172),   0,  2, 10, MvtKind::Fill},
    {"landcover",  "urban_area",     rgb565(225, 224, 224),   0,  1,  8, MvtKind::Fill},
    {"landcover",  "wood",           rgb565(173, 209, 158),   0,  2,  8, MvtKind::Fill},
    {"landcover",  nullptr,          rgb565(242, 239, 233),   0,  0,  8, MvtKind::Fill},
    {"landuse",    "aerodrome",      rgb565(231, 230, 222),   0,  1, 10, MvtKind::Fill},
    {"landuse",    "allotments",     rgb565(202, 226, 193),   0,  2, 10, MvtKind::Fill},
    {"landuse",    "bare_rock",      rgb565(238, 230, 221),   0,  6,  8, MvtKind::Fill},
    {"landuse",    "cemetery",       rgb565(170, 203, 175),   0,  3, 11, MvtKind::Fill},
    {"landuse",    "dam",            rgb565(173, 173, 173),   0,  6, 12, MvtKind::Fill},
    {"landuse",    "farmland",       rgb565(238, 240, 214),   0,  2,  9, MvtKind::Fill},
    {"landuse",    "forest",         rgb565(173, 209, 158),   0,  2,  8, MvtKind::Fill},
    {"landuse",    "grass",          rgb565(206, 236, 177),   0,  5,  9, MvtKind::Fill},
    {"landuse",    "grassland",      rgb565(206, 236, 177),   0,  5,  9, MvtKind::Fill},
    {"landuse",    "industrial",     rgb565(235, 219, 232),   0,  1, 10, MvtKind::Fill},
    {"landuse",    "meadow",         rgb565(206, 236, 177),   0,  5, 10, MvtKind::Fill},
    {"landuse",    "military",       rgb565(245, 191, 191),   0,  2, 10, MvtKind::Fill},
    {"landuse",    "nature_reserve", rgb565(242, 239, 233),   0,  1, 12, MvtKind::Fill},
    {"landuse",    "other",          rgb565(242, 239, 233),   0,  1, 10, MvtKind::Fill},
    {"landuse",    "park",           rgb565(199, 250, 203),   0,  4, 12, MvtKind::Fill},
    {"landuse",    "pitch",          rgb565(169, 224, 203),   0,  5, 13, MvtKind::Fill},
    {"landuse",    "playground",     rgb565(214, 255, 218),   0,  4, 12, MvtKind::Fill},
    {"landuse",    "residential",    rgb565(225, 224, 224),   0,  1,  8, MvtKind::Fill},
    {"landuse",    "runway",         rgb565(178, 181, 209),   0,  6, 12, MvtKind::Fill},
    {"landuse",    "school",         rgb565(255, 255, 229),   0,  4, 13, MvtKind::Fill},
    {"landuse",    "scrub",          rgb565(201, 215, 172),   0,  2, 10, MvtKind::Fill},
    {"landuse",    "wetland",        rgb565(172, 210, 191),   0,  4, 11, MvtKind::Fill},
    {"landuse",    "wood",           rgb565(173, 209, 158),   0,  2,  8, MvtKind::Fill},
    {"landuse",    "zoo",            rgb565(214, 255, 218),   0,  4, 10, MvtKind::Fill},
    {"landuse",    nullptr,          rgb565(242, 239, 233),   0,  1, 10, MvtKind::Fill},
    {"water",      "basin",          rgb565(170, 211, 223),   0,  7, 11, MvtKind::Fill},
    {"water",      "canal",          rgb565(170, 211, 223),   2,  7, 10, MvtKind::Line},
    {"water",      "lake",           rgb565(170, 211, 223),   0,  7,  6, MvtKind::Fill},
    {"water",      "ocean",          rgb565(170, 211, 223),   0,  7,  6, MvtKind::Fill},
    {"water",      "river",          rgb565(170, 211, 223),   2,  7,  8, MvtKind::Line},
    {"water",      "stream",         rgb565(170, 211, 223),   2,  7, 13, MvtKind::Line},
    {"water",      "swimming_pool",  rgb565(170, 211, 223),   0,  7, 14, MvtKind::Fill},
    {"water",      "water",          rgb565(170, 211, 223),   0,  7,  8, MvtKind::Fill},
    {"water",      nullptr,          rgb565(170, 211, 223),   0,  7,  6, MvtKind::Fill},
    {"buildings",  "building",       rgb565(217, 208, 201),   0,  7, 13, MvtKind::Fill},
    {"buildings",  "building_part",  rgb565(217, 208, 201),   0,  7, 14, MvtKind::Fill},
    {"buildings",  nullptr,          rgb565(217, 208, 201),   0,  7, 13, MvtKind::Fill},
    {"boundaries", "country",        rgb565(141, 97, 139),    3,  3,  6, MvtKind::Line},
    {"boundaries", "county",         rgb565(141, 97, 139),    2,  3,  9, MvtKind::Line},
    {"boundaries", "locality",       rgb565(141, 97, 139),    2,  3, 13, MvtKind::Line},
    {"boundaries", "region",         rgb565(141, 97, 139),    2,  3,  6, MvtKind::Line},
    {"boundaries", nullptr,          rgb565(141, 97, 139),    2,  3,  6, MvtKind::Line},
    {"roads",      "aeroway",        rgb565(178, 181, 209),   2,  6, 12, MvtKind::Line},
    {"roads",      "ferry",          rgb565(0, 0, 0),         0,  0,  0, MvtKind::None},
    {"roads",      "highway",        rgb565(232, 146, 162),   8, 14,  6, MvtKind::Fill},
    {"roads",      "major_road",     rgb565(252, 214, 164),   7, 13,  7, MvtKind::Fill},
    {"roads",      "minor_road",     rgb565(255, 255, 255),   4, 12, 12, MvtKind::Fill},
    {"roads",      "path",           rgb565(248, 133, 118),   2,  8, 14, MvtKind::Fill},
    {"roads",      "rail",           rgb565(136, 136, 136),   2, 15, 10, MvtKind::Fill},
    {"roads",      nullptr,          rgb565(255, 255, 255),   4, 12, 12, MvtKind::Line},
    {"places",     "locality",       rgb565(85, 85, 85),      0, 12,  8, MvtKind::Text},
    {"places",     "macrohood",      rgb565(102, 102, 102),   0, 12, 12, MvtKind::Text},
    {"places",     "neighbourhood",  rgb565(102, 102, 102),   0, 12, 13, MvtKind::Text},
};

/// Geometry scratch reused across features of the same pass. Kept in PSRAM: at
/// the renderer's point ceiling it is 64 KB, too much internal SRAM on the S3.
static std::vector<int32_t, PsramAllocator<int32_t>> mvtPointsX;
static std::vector<int32_t, PsramAllocator<int32_t>> mvtPointsY;
static std::vector<uint16_t, PsramAllocator<uint16_t>> mvtRingEnds;

/// Layer dictionaries. Static because a tile with hundreds of values does not
/// fit on the render task stack next to the decode call.
static const uint8_t* mvtKeySlices[128];
static size_t mvtKeyLengths[128];
static const uint8_t* mvtValueSlices[512];
static size_t mvtValueLengths[512];

/**
 * @brief Read an unsigned LEB128 varint from a memory buffer.
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
 * @brief Decode a zigzag-encoded delta.
 *
 * @param value Encoded value.
 * @return Signed delta.
 */
static int32_t decodeZigZag(int32_t value)
{
    return (value >> 1) ^ -(value & 1);
}

/**
 * @brief Append an unsigned varint to a buffer.
 *
 * @param dst    Destination cursor, advanced past the varint.
 * @param dstEnd End of the destination.
 * @param value  Value to write.
 * @return True when the value fitted.
 */
static bool writeVarint(uint8_t*& dst, uint8_t* dstEnd, uint32_t value)
{
    while (value > 0x7F)
    {
        if (dst >= dstEnd)
            return false;
        *dst++ = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    if (dst >= dstEnd)
        return false;
    *dst++ = (uint8_t)value;
    return true;
}

/**
 * @brief Find the style rule that applies to a layer and class.
 *
 * @param layer      Layer name of the MVT feature.
 * @param layerLen   Length of the layer name.
 * @param cls        Class or kind value, may be nullptr.
 * @param clsLen     Length of the class value.
 * @param out        Output rule.
 * @return True when a rule matched.
 */
static bool findStyle(const char* layer, size_t layerLen, const char* cls, size_t clsLen, MvtStyleRule& out)
{
    for (const MvtStyleRule& rule : MVT_STYLE)
    {
        if (strlen(rule.layer) != layerLen || strncmp(rule.layer, layer, layerLen) != 0)
            continue;
        if (rule.cls == nullptr)
        {
            out = rule;
            return true;
        }
        if (cls != nullptr && strlen(rule.cls) == clsLen && strncmp(rule.cls, cls, clsLen) == 0)
        {
            out = rule;
            return true;
        }
    }
    return false;
}

/**
 * @brief Report whether a layer has style rules and which one is its default.
 *
 * @details A layer without a default rule is only drawn feature by feature, when
 *          the class matches: `places` keeps the locality names and drops the
 *          neighbourhood ones, like the native packs do. A zeroed rule means the
 *          layer has no default, so anything that does not match a class is skipped.
 *
 * @param layer       Layer name of the MVT feature.
 * @param layerLen    Length of the layer name.
 * @param defaultRule Output: the default rule, zeroed when the layer has none.
 * @return True when the layer appears in the style table.
 */
static bool layerHasRules(const char* layer, size_t layerLen, MvtStyleRule& defaultRule)
{
    bool found = false;
    for (const MvtStyleRule& rule : MVT_STYLE)
    {
        if (strlen(rule.layer) != layerLen || strncmp(rule.layer, layer, layerLen) != 0)
            continue;
        found = true;
        if (rule.cls == nullptr)
            defaultRule = rule;
    }
    return found;
}

/**
 * @brief Fetch and inflate one MVT tile into the scratch buffer.
 *
 * @param tileX   Global tile column.
 * @param tileY   Global tile row.
 * @param zoom    Zoom level.
 * @param outSize Output: inflated size.
 * @return Pointer to the inflated tile, or nullptr.
 */
uint8_t* Maps::loadMvtTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, size_t& outSize)
{
    outSize = 0;
    if (!PmtilesReader::isOpen())
        return nullptr;

    uint64_t offset = 0;
    uint32_t length = 0;
    if (!PmtilesReader::findTile(tileX, tileY, zoom, offset, length))
        return nullptr;

    if (!ensureMvtBuffers())
        return nullptr;

    if (!PmtilesReader::readTile(offset, length, mvtTileBuffer, mvtTileBufferSize, outSize))
    {
        // The caller logs the tile and the reason, so a failed tile leaves one line.
        outSize = 0;
        return nullptr;
    }
    return mvtTileBuffer;
}

/**
 * @brief Allocate the MVT working buffers.
 *
 * @details The arena is taken first and as early as possible: a large
 *          compressed tile grows the reader buffer before the first decode, and
 *          that growth can take the contiguous block the arena needs.
 *
 * @return True when both buffers are ready.
 */
bool Maps::ensureMvtBuffers()
{
    if (!mvtArena)
    {
        if (!takeMvtArenaBlock())
        {
            ESP_LOGE(TAG, "no memory for the feature arena (%u B block)", (unsigned)MVT_ARENA_BLOCK_BYTES);
            return false;
        }
    }

    if (!mvtTileBuffer)
    {
        mvtTileBuffer = static_cast<uint8_t*>(heap_caps_malloc(MVT_TILE_BYTES, MALLOC_CAP_SPIRAM));
        if (!mvtTileBuffer)
            mvtTileBuffer = static_cast<uint8_t*>(heap_caps_malloc(MVT_TILE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (!mvtTileBuffer)
        {
            ESP_LOGE(TAG, "no memory for the MVT tile buffer (%u B)", (unsigned)MVT_TILE_BYTES);
            return false;
        }
        mvtTileBufferSize = MVT_TILE_BYTES;
    }

    return true;
}

/**
 * @brief Take the next arena block, allocating it on first use.
 */
bool Maps::takeMvtArenaBlock()
{
    if (mvtBlockIndex >= MVT_ARENA_BLOCK_COUNT)
        return false;

    if (mvtBlockIndex >= mvtBlockCount)
    {
        uint8_t* block = static_cast<uint8_t*>(heap_caps_malloc(MVT_ARENA_BLOCK_BYTES, MALLOC_CAP_SPIRAM));
        if (!block)
            block = static_cast<uint8_t*>(heap_caps_malloc(MVT_ARENA_BLOCK_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (!block)
            return false;
        mvtBlocks[mvtBlockCount] = block;
        mvtBlockCount++;
    }

    mvtArena = mvtBlocks[mvtBlockIndex];
    mvtBlockIndex++;
    mvtArenaSize = MVT_ARENA_BLOCK_BYTES;
    mvtArenaUsed = 0;
    return true;
}

/**
 * @brief Point the decoder back at the first block of the pass.
 */
void Maps::resetMvtArena()
{
    mvtBlockIndex = 0;
    mvtArena = nullptr;
    mvtArenaSize = 0;
    mvtArenaUsed = 0;
    if (mvtBlockCount > 0)
        takeMvtArenaBlock();
}

/**
 * @brief Release the MVT working buffers at the end of a pass.
 */
void Maps::releaseMvtMemory()
{
    // Blocks are only taken as the pass needs them, so the peak is the last one.
    if (mvtBlockCount > 0 && mvtBlockIndex > 0)
    {
        const size_t used = (size_t)(mvtBlockIndex - 1) * MVT_ARENA_BLOCK_BYTES + mvtArenaUsed;
        ESP_LOGI(TAG, "arena peak %u of %u B in %u blocks", (unsigned)used,
                 (unsigned)(mvtBlockCount * MVT_ARENA_BLOCK_BYTES), (unsigned)mvtBlockCount);
    }

    for (uint8_t block = 0; block < MVT_ARENA_BLOCK_COUNT; block++)
    {
        if (mvtBlocks[block])
            heap_caps_free(mvtBlocks[block]);
        mvtBlocks[block] = nullptr;
    }
    mvtBlockCount = 0;
    mvtBlockIndex = 0;
    mvtArena = nullptr;
    mvtArenaSize = 0;
    mvtArenaUsed = 0;

    if (mvtTileBuffer)
        heap_caps_free(mvtTileBuffer);
    mvtTileBuffer = nullptr;
    mvtTileBufferSize = 0;

    PmtilesReader::releaseBuffers();
}

/**
 * @brief Release the MVT working buffers and the feature lists of the pass.
 */
void Maps::releaseMvtBuffers()
{
    releaseMvtMemory();
    featurePool.clear();
    for (int i = 0; i < 16; i++)
    {
        layers[i].clear();
        layersCasing[i].clear();
        layersText[i].clear();
    }
}

/**
 * @brief Decode one MVT tile into the feature pool, in the NAV1 payload shape.
 *
 * @details Layers are walked twice: the first pass collects the name, keys,
 *          values and extent, which the protobuf encoding places after the
 *          features; the second pass decodes each feature. Geometry is
 *          re-encoded as delta plus zigzag varints and a trailing ring table, so
 *          the existing renderer works untouched. Attributes are dropped except
 *          the class used by the style table and the name used by labels. A
 *          polygon keeps all its rings in one feature, holes included, while a
 *          line is emitted as one feature per part: a border arrives split where
 *          it meets a tile edge or the coast, and the parts are strokes of the
 *          same line, not a tail to throw away.
 *
 * @param data     Inflated tile.
 * @param dataSize Inflated size.
 * @param screenX  Tile origin on the sprite, horizontal.
 * @param screenY  Tile origin on the sprite, vertical.
 * @param zoom     Zoom level.
 */
void Maps::decodeVectorFeaturesMvt(const uint8_t* data, size_t dataSize, int16_t screenX, int16_t screenY, uint8_t zoom)
{
    if (!data || dataSize == 0)
        return;

    esp_log_level_set(TAG, ESP_LOG_INFO);

    if (!ensureMvtBuffers())
        return;

    if (mvtPointsX.capacity() == 0)
    {
        mvtPointsX.reserve(MVT_MAX_POINTS);
        mvtPointsY.reserve(MVT_MAX_POINTS);
        mvtRingEnds.reserve(MVT_MAX_POINTS / 4);
    }

    bool arenaExhausted = false;
    uint16_t featuresAdded = 0;
    uint16_t styleDropped = 0;
    uint16_t geomDropped = 0;
    uint16_t poolDropped = 0;
    uint16_t truncatedCount = 0;

    const uint8_t* tileEnd = data + dataSize;
    const uint8_t* fieldCursor = data;
    while (fieldCursor < tileEnd)
    {
        uint64_t key = 0;
        if (!readVarint(fieldCursor, tileEnd, key))
            break;
        const uint32_t field = (uint32_t)(key >> 3);
        const uint32_t wire = (uint32_t)(key & 0x07);
        if (wire == 2)
        {
            uint64_t length = 0;
            if (!readVarint(fieldCursor, tileEnd, length) || length > (uint64_t)(tileEnd - fieldCursor))
                break;
            if (field == 3)
            {
                const uint8_t* layer = fieldCursor;
                const size_t layerLength = (size_t)length;

                const char* layerName = nullptr;
                size_t layerNameLength = 0;
                const uint8_t** keySlices = mvtKeySlices;
                size_t* keyLengths = mvtKeyLengths;
                uint16_t keyCount = 0;
                const uint8_t** valueSlices = mvtValueSlices;
                size_t* valueLengths = mvtValueLengths;
                uint16_t valueCount = 0;
                uint32_t extent = 4096;

                const uint8_t* scan = layer;
                const uint8_t* scanEnd = layer + layerLength;
                while (scan < scanEnd)
                {
                    uint64_t layerKey = 0;
                    if (!readVarint(scan, scanEnd, layerKey))
                        break;
                    const uint32_t layerField = (uint32_t)(layerKey >> 3);
                    const uint32_t layerWire = (uint32_t)(layerKey & 0x07);
                    if (layerWire == 2)
                    {
                        uint64_t length2 = 0;
                        if (!readVarint(scan, scanEnd, length2) || length2 > (uint64_t)(scanEnd - scan))
                            break;
                        if (layerField == 1)
                        {
                            layerName = reinterpret_cast<const char*>(scan);
                            layerNameLength = (size_t)length2;
                        }
                        else if (layerField == 3 && keyCount < 128)
                        {
                            keySlices[keyCount] = scan;
                            keyLengths[keyCount] = (size_t)length2;
                            keyCount++;
                        }
                        else if (layerField == 4 && valueCount < 512)
                        {
                            valueSlices[valueCount] = scan;
                            valueLengths[valueCount] = (size_t)length2;
                            valueCount++;
                        }
                        scan += length2;
                    }
                    else if (layerWire == 0)
                    {
                        uint64_t scalar = 0;
                        if (!readVarint(scan, scanEnd, scalar))
                            break;
                        if (layerField == 5 && scalar > 0)
                            extent = (uint32_t)scalar;
                    }
                    else if (layerWire == 1)
                    {
                        scan += 8;
                    }
                    else if (layerWire == 5)
                    {
                        scan += 4;
                    }
                    else
                    {
                        break;
                    }
                }

                MvtStyleRule defaultRule = {};
                const bool layerKnown = (layerName != nullptr) &&
                                        layerHasRules(layerName, layerNameLength, defaultRule);

                const uint8_t* featureCursor = layer;
                while (layerKnown && featureCursor < scanEnd)
                {
                    uint64_t layerKey = 0;
                    if (!readVarint(featureCursor, scanEnd, layerKey))
                        break;
                    const uint32_t layerField = (uint32_t)(layerKey >> 3);
                    const uint32_t layerWire = (uint32_t)(layerKey & 0x07);
                    if (layerWire == 2)
                    {
                        uint64_t length2 = 0;
                        if (!readVarint(featureCursor, scanEnd, length2) || length2 > (uint64_t)(scanEnd - featureCursor))
                            break;
                        if (layerField == 2)
                        {
                            const uint8_t* feature = featureCursor;
                            const uint8_t* featureEnd = featureCursor + length2;
                            uint32_t geomType = 0;
                            const uint8_t* tags = nullptr;
                            size_t tagsLength = 0;
                            const uint8_t* geometry = nullptr;
                            size_t geometryLength = 0;

                            const uint8_t* walk = feature;
                            while (walk < featureEnd)
                            {
                                uint64_t featureKey = 0;
                                if (!readVarint(walk, featureEnd, featureKey))
                                    break;
                                const uint32_t featureField = (uint32_t)(featureKey >> 3);
                                const uint32_t featureWire = (uint32_t)(featureKey & 0x07);
                                if (featureWire == 2)
                                {
                                    uint64_t length3 = 0;
                                    if (!readVarint(walk, featureEnd, length3) || length3 > (uint64_t)(featureEnd - walk))
                                        break;
                                    if (featureField == 2)
                                    {
                                        tags = walk;
                                        tagsLength = (size_t)length3;
                                    }
                                    else if (featureField == 4)
                                    {
                                        geometry = walk;
                                        geometryLength = (size_t)length3;
                                    }
                                    walk += length3;
                                }
                                else if (featureWire == 0)
                                {
                                    uint64_t scalar = 0;
                                    if (!readVarint(walk, featureEnd, scalar))
                                        break;
                                    if (featureField == 3)
                                        geomType = (uint32_t)scalar;
                                }
                                else if (featureWire == 1)
                                {
                                    walk += 8;
                                }
                                else if (featureWire == 5)
                                {
                                    walk += 4;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            MvtStyleRule rule = defaultRule;
                            const char* textPointer = nullptr;
                            size_t textLength = 0;
                            uint8_t textMinZoom = 0xFF;

                            if (tags != nullptr && tagsLength > 0)
                            {
                                const uint8_t* tagCursor = tags;
                                const uint8_t* tagEnd = tags + tagsLength;
                                uint64_t tagValue = 0;
                                uint32_t keyIndex = 0;
                                bool haveKey = false;
                                while (tagCursor < tagEnd)
                                {
                                    if (!readVarint(tagCursor, tagEnd, tagValue))
                                        break;
                                    if (!haveKey)
                                    {
                                        keyIndex = (uint32_t)tagValue;
                                        haveKey = true;
                                        continue;
                                    }
                                    haveKey = false;
                                    if (keyIndex >= keyCount || tagValue >= valueCount)
                                        continue;

                                    const char* name = reinterpret_cast<const char*>(keySlices[keyIndex]);
                                    const size_t nameLength = keyLengths[keyIndex];
                                    const uint8_t* value = valueSlices[tagValue];
                                    const uint8_t* valueEnd = value + valueLengths[tagValue];

                                    const uint8_t* stringCursor = value;
                                    uint64_t valueKey = 0;
                                    if (!readVarint(stringCursor, valueEnd, valueKey))
                                        continue;
                                    const uint32_t valueField = (uint32_t)(valueKey >> 3);
                                    const uint32_t valueWire = (uint32_t)(valueKey & 0x07);
                                    // `min_zoom` is the zoom at which the basemap's own cartography
                                    // starts to label the place: an integer, not a string, so it has to
                                    // be read before the string-only path below drops it. It lets a
                                    // capital show at z6 without labelling every village, and it never
                                    // pulls a label later than the class rule does.
                                    if (valueWire == 0 && valueField == 5 &&
                                        nameLength == 8 && strncmp(name, "min_zoom", 8) == 0)
                                    {
                                        uint64_t placeMinZoom = 0;
                                        if (readVarint(stringCursor, valueEnd, placeMinZoom) && placeMinZoom < 32)
                                            textMinZoom = (uint8_t)placeMinZoom;
                                        continue;
                                    }
                                    if (valueField != 1 || valueWire != 2)
                                        continue;
                                    uint64_t stringLength = 0;
                                    if (!readVarint(stringCursor, valueEnd, stringLength))
                                        continue;
                                    if (stringLength > (uint64_t)(valueEnd - stringCursor))
                                        continue;

                                    const bool isClass = (nameLength == 5 && strncmp(name, "class", 5) == 0) ||
                                                         (nameLength == 4 && strncmp(name, "kind", 4) == 0) ||
                                                         (nameLength == 8 && strncmp(name, "subclass", 8) == 0);
                                    if (isClass)
                                        findStyle(layerName, layerNameLength,
                                                  reinterpret_cast<const char*>(stringCursor), (size_t)stringLength, rule);
                                    if (nameLength == 4 && strncmp(name, "name", 4) == 0)
                                    {
                                        textPointer = reinterpret_cast<const char*>(stringCursor);
                                        textLength = (size_t)stringLength;
                                    }
                                }
                            }

                            uint8_t effectiveMinZoom = rule.minZoom;
                            if (rule.kind == MvtKind::Text && textMinZoom < effectiveMinZoom)
                                effectiveMinZoom = textMinZoom;
                            if (rule.layer == nullptr || zoom < effectiveMinZoom || rule.kind == MvtKind::None)
                            {
                                styleDropped++;
                                featureCursor += length2;
                                continue;
                            }

                            if (rule.kind == MvtKind::Text)
                            {
                                if (textPointer == nullptr || textLength == 0 || geometry == nullptr)
                                {
                                    geomDropped++;
                                    featureCursor += length2;
                                    continue;
                                }
                                if (!mvtPointsX.empty())
                                {
                                    mvtPointsX.clear();
                                    mvtPointsY.clear();
                                }
                                int32_t cursorX = 0;
                                int32_t cursorY = 0;
                                const uint8_t* geom = geometry;
                                const uint8_t* geomEnd = geometry + geometryLength;
                                uint64_t command = 0;
                                if (!readVarint(geom, geomEnd, command))
                                {
                                    featureCursor += length2;
                                    continue;
                                }
                                uint64_t deltaX = 0;
                                uint64_t deltaY = 0;
                                if (!readVarint(geom, geomEnd, deltaX) || !readVarint(geom, geomEnd, deltaY))
                                {
                                    featureCursor += length2;
                                    continue;
                                }
                                cursorX += decodeZigZag((int32_t)deltaX);
                                cursorY += decodeZigZag((int32_t)deltaY);

                                size_t textSize = textLength;
                                if (textSize > 127)
                                    textSize = 127;
                                const size_t payloadSize = 5 + textSize;
                                if (mvtArenaUsed + payloadSize > mvtArenaSize && !takeMvtArenaBlock())
                                {
                                    arenaExhausted = true;
                                    featureCursor += length2;
                                    continue;
                                }
                                uint8_t* payload = mvtArena + mvtArenaUsed;
                                mvtArenaUsed += payloadSize;

                                const int16_t px = (int16_t)((int64_t)cursorX * 4096 / (int64_t)extent);
                                const int16_t py = (int16_t)((int64_t)cursorY * 4096 / (int64_t)extent);
                                memcpy(payload, &px, 2);
                                memcpy(payload + 2, &py, 2);
                                payload[4] = (uint8_t)textSize;
                                memcpy(payload + 5, textPointer, textSize);

                                if (featurePool.size() < MAX_FEATURE_POOL_SIZE)
                                {
                                    const uint16_t poolIndex = (uint16_t)featurePool.size();
                                    featurePool.push_back({payload, NavGeomType::Text, (uint16_t)payloadSize, 1,
                                                           screenX, screenY, rule.color, rule.width, false,
                                                           0, 0, 0, 0, rule.priority, false});
                                    layersText[rule.priority].push_back(poolIndex);
                                    featuresAdded++;
                                }
                                else
                                {
                                    poolDropped++;
                                }
                                featureCursor += length2;
                                continue;
                            }

                            if (geometry == nullptr || geometryLength == 0)
                            {
                                geomDropped++;
                                featureCursor += length2;
                                continue;
                            }

                            const NavGeomType navType = (geomType == 3) ? NavGeomType::Polygon : NavGeomType::LineString;
                            const bool isPolygon = (navType == NavGeomType::Polygon);
                            bool featureTruncated = false;

                            mvtPointsX.clear();
                            mvtPointsY.clear();
                            mvtRingEnds.clear();

                            int32_t cursorX = 0;
                            int32_t cursorY = 0;
                            const uint8_t* geom = geometry;
                            const uint8_t* geomEnd = geometry + geometryLength;
                            bool geometryOk = true;
                            while (geom < geomEnd)
                            {
                                uint64_t command = 0;
                                if (!readVarint(geom, geomEnd, command))
                                {
                                    geometryOk = false;
                                    break;
                                }
                                const uint32_t commandId = (uint32_t)(command & 0x07);
                                const uint32_t commandCount = (uint32_t)(command >> 3);
                                if (commandId == 7)
                                {
                                    if (isPolygon && !mvtPointsX.empty() &&
                                        (mvtRingEnds.empty() || mvtRingEnds.back() != (uint16_t)mvtPointsX.size()))
                                        mvtRingEnds.push_back((uint16_t)mvtPointsX.size());
                                    continue;
                                }
                                if (commandId != 1 && commandId != 2)
                                {
                                    geometryOk = false;
                                    break;
                                }
                                // A second MoveTo opens the next part of a multi-part
                                // line: the border arrives split into parts, so each one
                                // is recorded and decoded as its own feature. Ending the
                                // feature here kept only the first part, which at z6 and
                                // z7 is a handful of pixels of a border that is 12 parts.
                                if (!isPolygon && commandId == 1 && !mvtPointsX.empty())
                                    mvtRingEnds.push_back((uint16_t)mvtPointsX.size());
                                for (uint32_t i = 0; i < commandCount; i++)
                                {
                                    uint64_t deltaX = 0;
                                    uint64_t deltaY = 0;
                                    if (!readVarint(geom, geomEnd, deltaX) || !readVarint(geom, geomEnd, deltaY))
                                    {
                                        geometryOk = false;
                                        break;
                                    }
                                    cursorX += decodeZigZag((int32_t)deltaX);
                                    cursorY += decodeZigZag((int32_t)deltaY);
                                    const int32_t scaledX = (int32_t)((int64_t)cursorX * 4096 / (int64_t)extent);
                                    const int32_t scaledY = (int32_t)((int64_t)cursorY * 4096 / (int64_t)extent);
                                    // A vertex that lands on the pixel of the previous one
                                    // draws nothing at 256 px, and the tile comes with far
                                    // more vertices than pixels: dropping it here keeps a
                                    // dense ring whole instead of truncating its tail.
                                    if (!mvtPointsX.empty() &&
                                        (int32_t)(mvtPointsX.back() >> 4) == (int32_t)(scaledX >> 4) &&
                                        (int32_t)(mvtPointsY.back() >> 4) == (int32_t)(scaledY >> 4))
                                        continue;
                                    if (mvtPointsX.size() >= MVT_MAX_POINTS)
                                    {
                                        // Out of budget mid ring: the feature ends here,
                                        // with the ring closed where it stops. Keeping the
                                        // remaining vertices without their predecessors
                                        // filled a shape that was not the polygon.
                                        featureTruncated = true;
                                        break;
                                    }
                                    mvtPointsX.push_back(scaledX);
                                    mvtPointsY.push_back(scaledY);
                                }
                                if (!geometryOk || featureTruncated)
                                    break;
                            }

                            const uint16_t coordCount = (uint16_t)mvtPointsX.size();
                            const bool tooFew = isPolygon ? (coordCount < 3) : (coordCount < 2);
                            if (!geometryOk || tooFew)
                            {
                                geomDropped++;
                                featureCursor += length2;
                                continue;
                            }
                            if (featureTruncated)
                                truncatedCount++;

                            if (mvtRingEnds.empty() || mvtRingEnds.back() != coordCount)
                                mvtRingEnds.push_back(coordCount);

                            // Writes one feature from a slice of the decoded points.
                            // A polygon comes with its ring table; a line comes with none
                            // and is emitted part by part, since the payload is delta
                            // encoded from the start of the slice.
                            auto emitSlice = [&](uint16_t sliceStart, uint16_t sliceCount,
                                                 uint16_t ringCount) -> bool
                            {
                                // Worst case per point is a pair of zigzag varints and the
                                // ring table closes the payload. Asking for that room up
                                // front is what keeps a feature from being written half way;
                                // when the block has no room left, the next one takes it.
                                const size_t payloadNeed = (size_t)sliceCount * 11 + 4 + (size_t)ringCount * 2;
                                if (mvtArenaUsed + payloadNeed > mvtArenaSize)
                                    takeMvtArenaBlock();

                                uint8_t* payload = mvtArena + mvtArenaUsed;
                                uint8_t* payloadEnd = mvtArena + mvtArenaSize;
                                uint8_t* write = payload;
                                int32_t lastX = 0;
                                int32_t lastY = 0;
                                int16_t minPx = 255;
                                int16_t minPy = 255;
                                int16_t maxPx = 0;
                                int16_t maxPy = 0;
                                bool writeOk = true;

                                for (uint16_t i = 0; i < sliceCount && writeOk; i++)
                                {
                                    const int32_t scaledX = mvtPointsX[sliceStart + i];
                                    const int32_t scaledY = mvtPointsY[sliceStart + i];
                                    const int32_t deltaX = scaledX - lastX;
                                    const int32_t deltaY = scaledY - lastY;
                                    lastX = scaledX;
                                    lastY = scaledY;

                                    writeOk = writeVarint(write, payloadEnd, ((uint32_t)deltaX << 1) ^ (uint32_t)(deltaX >> 31)) &&
                                              writeVarint(write, payloadEnd, ((uint32_t)deltaY << 1) ^ (uint32_t)(deltaY >> 31));

                                    int16_t px = (int16_t)(scaledX >> 4);
                                    int16_t py = (int16_t)(scaledY >> 4);
                                    if (px < 0)
                                        px = 0;
                                    if (px > 255)
                                        px = 255;
                                    if (py < 0)
                                        py = 0;
                                    if (py > 255)
                                        py = 255;
                                    if (px < minPx)
                                        minPx = px;
                                    if (px > maxPx)
                                        maxPx = px;
                                    if (py < minPy)
                                        minPy = py;
                                    if (py > maxPy)
                                        maxPy = py;
                                }

                                if (writeOk && ringCount > 0)
                                {
                                    if (write + 2 + (size_t)ringCount * 2 > payloadEnd)
                                        writeOk = false;
                                    if (writeOk)
                                    {
                                        write[0] = (uint8_t)(ringCount & 0xFF);
                                        write[1] = (uint8_t)(ringCount >> 8);
                                        write += 2;
                                        for (uint16_t r = 0; r < ringCount; r++)
                                        {
                                            write[0] = (uint8_t)(mvtRingEnds[r] & 0xFF);
                                            write[1] = (uint8_t)(mvtRingEnds[r] >> 8);
                                            write += 2;
                                        }
                                    }
                                }

                                if (!writeOk)
                                    return false;

                                const uint16_t payloadSize = (uint16_t)(write - payload);
                                mvtArenaUsed += payloadSize;

                                if (featurePool.size() < MAX_FEATURE_POOL_SIZE)
                                {
                                    const uint16_t poolIndex = (uint16_t)featurePool.size();
                                    featurePool.push_back({payload, navType, payloadSize, sliceCount,
                                                           screenX, screenY, rule.color, rule.width, false,
                                                           (uint8_t)minPx, (uint8_t)minPy, (uint8_t)maxPx, (uint8_t)maxPy,
                                                           rule.priority, false});
                                    layers[rule.priority].push_back(poolIndex);
                                    featuresAdded++;
                                }
                                else
                                {
                                    poolDropped++;
                                }
                                return true;
                            };

                            bool sliceOk = true;
                            if (isPolygon)
                            {
                                // One feature carries every ring: the holes belong to it.
                                sliceOk = emitSlice(0, coordCount, (uint16_t)mvtRingEnds.size());
                            }
                            else
                            {
                                // One feature per part: a border that arrives split into
                                // twelve parts is a line drawn in twelve strokes, not the
                                // first of them and nothing else.
                                uint16_t sliceStart = 0;
                                for (uint16_t r = 0; r < (uint16_t)mvtRingEnds.size() && sliceOk; r++)
                                {
                                    const uint16_t sliceEnd = mvtRingEnds[r];
                                    const uint16_t sliceCount = (uint16_t)(sliceEnd - sliceStart);
                                    if (sliceCount >= 2)
                                        sliceOk = emitSlice(sliceStart, sliceCount, 0);
                                    sliceStart = sliceEnd;
                                }
                            }
                            if (!sliceOk)
                            {
                                arenaExhausted = true;
                                featureCursor += length2;
                                continue;
                            }
                        }
                        featureCursor += length2;
                    }
                    else if (layerWire == 0)
                    {
                        uint64_t scalar = 0;
                        if (!readVarint(featureCursor, scanEnd, scalar))
                            break;
                    }
                    else if (layerWire == 1)
                    {
                        featureCursor += 8;
                    }
                    else if (layerWire == 5)
                    {
                        featureCursor += 4;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            fieldCursor += length;
        }
        else if (wire == 0)
        {
            uint64_t scalar = 0;
            if (!readVarint(fieldCursor, tileEnd, scalar))
                break;
        }
        else if (wire == 1)
        {
            fieldCursor += 8;
        }
        else if (wire == 5)
        {
            fieldCursor += 4;
        }
        else
        {
            break;
        }
    }

    if (arenaExhausted)
        ESP_LOGW(TAG, "feature arena exhausted (%u of %u B used), features dropped",
                 (unsigned)mvtArenaUsed, (unsigned)mvtArenaSize);

    // A tile that draws nothing, loses features or truncates a ring explains a
    // hole in the viewport, so it is worth one line per offending tile.
    if (featuresAdded == 0 || truncatedCount > 0 || poolDropped > 0)
        ESP_LOGW(TAG, "z%u tile at %d,%d: %u features | dropped style %u geom %u pool %u | truncated %u",
                 (unsigned)zoom, screenX, screenY, featuresAdded, styleDropped, geomDropped,
                 poolDropped, truncatedCount);
}
