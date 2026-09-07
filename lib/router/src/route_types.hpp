/**
 * @file route_types.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin binary format structs (0.1° subcell grid)
 * @version 0.3.0
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
    uint32_t turn_count;    // number of TurnRestriction entries after the data block
    uint32_t reserved[4];
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
    int16_t  lat_off;       // (lat − cell_center_lat) / 0.05° × 65536, clamp ±32767
    int16_t  lon_off;       // (lon − cell_center_lon) / 0.05° × 65536, clamp ±32767
    uint32_t edge_offset;   // relative to this cell's edge block
};
static_assert(sizeof(RouteNode) == 8, "RouteNode size mismatch");

// Rebuild absolute degrees from cell SW corner (lat_e4/lon_e4) + node offset.
// Matches the generator's quantization: offset 0 = cell centre, 1 step ≈ 0.085 m.
static inline float nodeLatDeg(int32_t cell_lat_e4, int16_t lat_off)
{
    return (cell_lat_e4 + 250) / 10000.0f + lat_off * (0.05f / 65536.0f);
}
static inline float nodeLonDeg(int32_t cell_lon_e4, int16_t lon_off)
{
    return (cell_lon_e4 + 250) / 10000.0f + lon_off * (0.05f / 65536.0f);
}

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

// Turn restriction: prohibits travelling in_edge -> out_edge through via_node.
// Edge indices are global (see route_generator.md).
struct TurnRestriction
{
    uint32_t via_node;   // global node index
    uint32_t in_edge;    // global edge index (entry)
    uint32_t out_edge;   // global edge index (forbidden exit)
};
static_assert(sizeof(TurnRestriction) == 12, "TurnRestriction size mismatch");


