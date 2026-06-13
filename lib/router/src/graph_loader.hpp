/**
 * @file graph_loader.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin paged graph loader with on-demand PSRAM cache
 * @version 0.2.9
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
    bool     isLoaded() const { return loaded_; }

    uint32_t nearestNode(float lat, float lon) const;
    bool     getNode(uint32_t gi, RouteNode& out_node) const;
    bool     getEdgesForNode(uint32_t gi, RouteEdge* buf, uint32_t& count) const;
    uint32_t totalNodes() const { return totalNodes_; }
    void     preloadPoint(float lat, float lon) const;

private:
    struct PageData
    {
        std::vector<RouteNode, PsramAllocator<RouteNode>> nodes;
        std::vector<RouteEdge, PsramAllocator<RouteEdge>> edges;
        uint32_t cell_idx;      // index into cellIndex_
        uint32_t lru_stamp;     // incremented on each access
    };

    using PageMap = std::unordered_map<uint32_t, PageData,
                        std::hash<uint32_t>, std::equal_to<uint32_t>,
                        PsramAllocator<std::pair<const uint32_t, PageData>>>;

    std::vector<CellIndexEntry> cellIndex_;
    mutable PageMap             pageCache_;
    mutable uint32_t            lru_clock_ = 0;

    mutable FILE*   file_               = nullptr;
    uint32_t        data_base_offset_   = 0;   // byte offset in file where cell data blocks start
    uint32_t        totalNodes_         = 0;
    bool            loaded_             = false;

    // Returns cell index for the global node gi, or UINT32_MAX if not found.
    uint32_t cellForNode(uint32_t gi) const;

    // Ensures the page for cell_idx is loaded in pageCache_. Returns pointer or nullptr.
    PageData* fetchPage(uint32_t cell_idx) const;

    // Evict the least-recently-used page.
    void evictLRU() const;

};

extern GraphLoader graphLoader;
