/**
 * @file graph_loader.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin paged graph loader with on-demand PSRAM cache
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once
#include <cstdio>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "route_types.hpp"
#include "PsramAllocator.hpp"

static constexpr uint32_t MAX_EDGES_PER_NODE_GL = 64;

// Maximum number of pages (subcells) to keep in PSRAM cache simultaneously.
// Each page holds nodes + edges for one 0.1°×0.1° cell.
// At ~1 MB free PSRAM and ~10-20 KB/page average, 48 pages ≈ 500–960 KB.
static constexpr uint32_t PAGE_CACHE_MAX = 48;

class GraphLoader
{
public:
    bool     load();
    void     unload();
    bool     isLoaded() const { return loaded; }

    uint32_t nearestNode(float lat, float lon) const;
    bool     getNode(uint32_t gi, RouteNode& out_node) const;
    bool     getNodeCoords(uint32_t gi, float& lat, float& lon) const;
    bool     getEdgesForNode(uint32_t gi, RouteEdge* buf, uint32_t& count) const;
    void     preloadPoint(float lat, float lon) const;

    // Turn restrictions (global edge index space).
    uint32_t edgeGlobalOffset(uint32_t cell_idx, uint32_t rel_edge) const;
    uint32_t edgeGlobalForNode(uint32_t gi, uint32_t rel_edge) const;
    bool     isTurnForbidden(uint32_t via_node, uint32_t in_edge, uint32_t out_edge) const;

private:
    struct PageData
    {
        std::vector<RouteNode, PsramAllocator<RouteNode>> nodes;
        std::vector<RouteEdge, PsramAllocator<RouteEdge>> edges;
        uint32_t cell_idx;      // index into cellIndex
        uint32_t lru_stamp;     // incremented on each access
    };

    using PageMap = std::unordered_map<uint32_t, PageData,
                        std::hash<uint32_t>, std::equal_to<uint32_t>,
                        PsramAllocator<std::pair<const uint32_t, PageData>>>;

    std::vector<CellIndexEntry> cellIndex;
    std::vector<TurnRestriction> turnRestrictions;
    std::vector<uint32_t>       cellEdgeBase;   // global edge index base per cell
    mutable PageMap             pageCache;
    mutable uint32_t            lru_clock = 0;

    mutable FILE*   file               = nullptr;
    uint32_t        data_base_offset   = 0;   // byte offset in file where cell data blocks start
    uint32_t        nodeCount         = 0;
    bool            loaded             = false;

    // Returns cell index for the global node gi, or UINT32_MAX if not found.
    uint32_t cellForNode(uint32_t gi) const;

    // Ensures the page for cell_idx is loaded in pageCache. Returns pointer or nullptr.
    PageData* fetchPage(uint32_t cell_idx) const;

    // Evict the least-recently-used page.
    void evictLRU() const;

};

