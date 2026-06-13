/**
 * @file route_types.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin binary format structs (0.1° subcell grid)
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once
#include <cstdint>

static constexpr char ROUTE_MAGIC[4] = {'R','O','U','T'};

// Returns the ROUTE.bin path for the given max-speed preference (km/h).
// Must match the subdirectory layout produced by route_generator: CAR / BIKE / WALK.
static inline const char* routeBinPath(uint16_t routeSpeed)
{
    if (routeSpeed <= 5)
        return "/sdcard/ROUTE/WALK/ROUTE.bin";
    if (routeSpeed <= 25)
        return "/sdcard/ROUTE/BIKE/ROUTE.bin";
    return "/sdcard/ROUTE/CAR/ROUTE.bin";
}

#pragma pack(push, 1)

// File header — 32 bytes
struct RouteFileHeader
{
    char     magic[4];
    uint32_t sub_step_e4;   // 500 = 0.05° cells
    uint32_t cell_count;
    uint32_t reserved[5];
};
static_assert(sizeof(RouteFileHeader) == 32, "RouteFileHeader size mismatch");

// Per-cell index entry — 20 bytes
struct CellIndexEntry
{
    int32_t  lat_e4;         // lat × 10000, snapped to 0.1° grid
    int32_t  lon_e4;         // lon × 10000, snapped to 0.1° grid
    uint32_t node_offset;    // global index of first node (for cellForNode / nearestNode)
    uint16_t node_count;
    uint32_t data_offset;    // byte offset from start of data block: cell stores [nodes][edges] contiguously
    uint16_t edge_count;
};
static_assert(sizeof(CellIndexEntry) == 20, "CellIndexEntry size mismatch");

struct RouteNode
{
    float    lat;
    float    lon;
    uint32_t edge_offset;    // relative to this cell's edge block
};
static_assert(sizeof(RouteNode) == 12, "RouteNode size mismatch");

struct RouteEdge
{
    uint32_t dst_node;       // global node index (absolute)
    uint32_t cost;           // tenths of second
    uint16_t dist_m;
    uint8_t  flags;
    uint8_t  reserved;
};
static_assert(sizeof(RouteEdge) == 12, "RouteEdge size mismatch");

#pragma pack(pop)

static inline uint8_t edge_highway_class(uint8_t f) { return (f >> 1) & 0x07; }
static inline bool    edge_is_oneway(uint8_t f)     { return (f & 0x01) != 0; }
