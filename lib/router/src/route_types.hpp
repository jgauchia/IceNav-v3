/**
 * @file route_types.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin v2 binary format structs
 * @version 0.2.6
 * @date 2026-05
 */

#pragma once
#include <cstdint>

static constexpr char     ROUTE_MAGIC[4]   = {'R','O','U','T'};
static constexpr uint8_t  ROUTE_VERSION    = 2;
static constexpr char     ROUTE_BIN_PATH[] = "/sdcard/ROUTE/ROUTE.bin";

#pragma pack(push, 1)

struct RouteFileHeader
{
    char     magic[4];
    uint8_t  version;
    uint8_t  reserved[3];
    uint32_t cell_count;
    uint32_t reserved2[4];
};
static_assert(sizeof(RouteFileHeader) == 28, "RouteFileHeader size mismatch");

struct CellIndexEntry
{
    int16_t  lat_floor;
    int16_t  lon_floor;
    uint32_t node_offset;  // global index of first node in this cell
    uint32_t node_count;
    uint32_t edge_offset;  // global index of first edge in this cell
    uint32_t edge_count;
    uint32_t reserved;
};
static_assert(sizeof(CellIndexEntry) == 24, "CellIndexEntry size mismatch");

struct RouteNode
{
    float    lat;
    float    lon;
    uint32_t edge_offset;  // relative to this cell's edge block
};
static_assert(sizeof(RouteNode) == 12, "RouteNode size mismatch");

struct RouteEdge
{
    uint32_t dst_node;     // global node index
    uint32_t cost;         // tenths of second
    uint16_t dist_m;
    uint8_t  flags;
    uint16_t name_idx;
    uint8_t  reserved;
};
static_assert(sizeof(RouteEdge) == 14, "RouteEdge size mismatch");

#pragma pack(pop)

static inline uint8_t edge_highway_class(uint8_t f) { return (f >> 1) & 0x07; }
static inline bool    edge_is_oneway(uint8_t f)     { return (f & 0x01) != 0; }
