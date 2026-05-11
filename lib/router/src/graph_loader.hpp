/**
 * @file graph_loader.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin v2 graph loader — active-cell nodes in PSRAM, edges via persistent FILE*
 * @version 0.2.6
 * @date 2026-05
 */

#pragma once
#include <cstdio>
#include <cstdint>
#include <vector>
#include "route_types.hpp"
#include "PsramAllocator.hpp"

static constexpr uint32_t MAX_EDGES_PER_NODE_GL = 64;

class GraphLoader
{
public:
    bool     load(float src_lat, float src_lon, float dst_lat, float dst_lon);
    bool     buildEdgeCache(float src_lat, float src_lon, float dst_lat, float dst_lon,
                            float margin = 0.05f);
    void     unload();
    bool     isLoaded() const { return loaded_; }

    uint32_t nearestNode(float lat, float lon) const;
    void     edgeForNode(uint32_t gi, uint32_t& e_start, uint32_t& e_end) const;
    bool     getNode(uint32_t gi, RouteNode& out_node) const;
    bool     getEdge(uint32_t ei, RouteEdge& out_edge) const;
    bool     getEdgesForNode(uint32_t gi, RouteEdge* buf, uint32_t& count) const;
    uint32_t totalNodes() const { return totalNodes_; }

private:
    std::vector<CellIndexEntry> cellIndex_;

    // Nodes of active cells cached in PSRAM
    std::vector<RouteNode, PsramAllocator<RouteNode>> nodeCache_;
    uint32_t nodeCache_base_  = 0;
    uint32_t nodeCache_count_ = 0;

    // Edges for the src↔dst bbox window cached in PSRAM (best-effort)
    std::vector<RouteEdge, PsramAllocator<RouteEdge>> edgeCache_;
    uint32_t edgeCache_base_  = 0;
    uint32_t edgeCache_count_ = 0;

    // Persistent FILE* kept open for the lifetime of a routing session
    mutable FILE* file_ = nullptr;

    uint32_t nodes_base_offset_ = 0;
    uint32_t edges_base_offset_ = 0;
    uint32_t totalNodes_        = 0;
    bool     loaded_            = false;
};

extern GraphLoader graphLoader;
