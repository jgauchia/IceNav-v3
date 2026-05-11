/**
 * @file graph_loader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin v2 graph loader — active-cell nodes in PSRAM, edges via persistent FILE*
 * @version 0.2.6
 * @date 2026-05
 */

#include "graph_loader.hpp"
#include "storage.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include "esp_heap_caps.h"

extern Storage storage;

GraphLoader graphLoader;

/**
 * @brief Load graph index and cache active-cell nodes from ROUTE.bin.
 *
 * Opens the file, reads the header and cell index, caches nodes for the
 * cells nearest to src and dst in PSRAM, and keeps the FILE* open for
 * subsequent edge reads.
 *
 * @param src_lat Source latitude in degrees
 * @param src_lon Source longitude in degrees
 * @param dst_lat Destination latitude in degrees
 * @param dst_lon Destination longitude in degrees
 * @return true on success, false if file missing or format error
 */
bool GraphLoader::load(float src_lat, float src_lon, float dst_lat, float dst_lon)
{
    unload();

    FILE* f = storage.open(ROUTE_BIN_PATH, "rb");
    if (!f)
    {
        printf("DEBUG GL: cannot open %s\n", ROUTE_BIN_PATH);
        return false;
    }

    RouteFileHeader hdr;
    if (storage.read(f, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) != sizeof(hdr))
    {
        storage.close(f);
        return false;
    }

    if (memcmp(hdr.magic, ROUTE_MAGIC, 4) != 0 || hdr.version != ROUTE_VERSION)
    {
        printf("DEBUG GL: bad magic or version\n");
        storage.close(f);
        return false;
    }

    cellIndex_.resize(hdr.cell_count);
    storage.read(f, reinterpret_cast<uint8_t*>(cellIndex_.data()), hdr.cell_count * sizeof(CellIndexEntry));

    nodes_base_offset_ = sizeof(RouteFileHeader) + hdr.cell_count * sizeof(CellIndexEntry);

    totalNodes_ = 0;
    for (const auto& c : cellIndex_)
    {
        uint32_t end = c.node_offset + c.node_count;
        if (end > totalNodes_) totalNodes_ = end;
    }
    edges_base_offset_ = nodes_base_offset_ + totalNodes_ * sizeof(RouteNode);

    printf("DEBUG GL: loading cells=%lu, total_nodes=%lu\n", (unsigned long)hdr.cell_count, (unsigned long)totalNodes_);

    // Try to cache nodes in PSRAM with decreasing bbox margins (0° → 0.5° → 1°).
    // Each attempt checks free PSRAM before allocating to avoid a guaranteed failure.
    static const float MARGINS[] = {0.0f, 0.5f, 1.0f};
    bool cacheLoaded = false;

    for (float margin : MARGINS)
    {
        float blmin = floorf(fminf(src_lat, dst_lat)) - margin;
        float blmax = floorf(fmaxf(src_lat, dst_lat)) + margin;
        float bnmin = floorf(fminf(src_lon, dst_lon)) - margin;
        float bnmax = floorf(fmaxf(src_lon, dst_lon)) + margin;

        uint32_t cache_first = totalNodes_;
        uint32_t cache_last  = 0;

        for (const auto& c : cellIndex_)
        {
            if (c.lat_floor < (int16_t)blmin || c.lat_floor > (int16_t)blmax) continue;
            if (c.lon_floor < (int16_t)bnmin || c.lon_floor > (int16_t)bnmax) continue;
            if (c.node_offset < cache_first) cache_first = c.node_offset;
            uint32_t end = c.node_offset + c.node_count;
            if (end > cache_last) cache_last = end;
        }

        if (cache_first >= cache_last) continue;

        uint32_t count  = cache_last - cache_first;
        size_t   needed = (size_t)count * sizeof(RouteNode);
        size_t   avail  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        printf("DEBUG GL: margin=%.1f -> %lu nodes (%lu KB), PSRAM free=%lu KB\n",
               margin, (unsigned long)count, (unsigned long)(needed / 1024), (unsigned long)(avail / 1024));

        if (needed + (256 * 1024) > avail)
        {
            printf("DEBUG GL: skip margin=%.1f (not enough PSRAM)\n", margin);
            continue;
        }

        try { nodeCache_.resize(count); }
        catch (const std::bad_alloc&)
        {
            nodeCache_.clear();
            printf("DEBUG GL: bad_alloc at margin=%.1f\n", margin);
            continue;
        }

        storage.seekAndRead(f, nodes_base_offset_ + cache_first * sizeof(RouteNode),
                            reinterpret_cast<uint8_t*>(nodeCache_.data()), needed);
        nodeCache_base_  = cache_first;
        nodeCache_count_ = count;
        printf("DEBUG GL: nodeCache OK — %lu nodes, margin=%.1f\n", (unsigned long)count, margin);
        cacheLoaded = true;
        break;
    }

    if (!cacheLoaded)
        printf("DEBUG GL: no PSRAM cache — all node reads will hit SD\n");

    // Keep the file open for edge reads during A*
    file_   = f;
    loaded_ = true;
    return true;
}

/**
 * @brief Read a single node — from PSRAM cache if available, otherwise from SD.
 *
 * @param gi Global node index
 * @param out_node Output node
 * @return true if successful
 */
bool GraphLoader::getNode(uint32_t gi, RouteNode& out_node) const
{
    if (nodeCache_count_ > 0 && gi >= nodeCache_base_ && gi < nodeCache_base_ + nodeCache_count_)
    {
        out_node = nodeCache_[gi - nodeCache_base_];
        return true;
    }
    if (!file_) return false;
    return storage.seekAndRead(file_, nodes_base_offset_ + gi * sizeof(RouteNode),
                               reinterpret_cast<uint8_t*>(&out_node), sizeof(RouteNode)) == sizeof(RouteNode);
}

/**
 * @brief Read a single edge directly from SD via the persistent FILE*.
 *
 * @param ei Global edge index
 * @param out_edge Output edge
 * @return true if successful
 */
bool GraphLoader::getEdge(uint32_t ei, RouteEdge& out_edge) const
{
    if (!file_) return false;
    return storage.seekAndRead(file_, edges_base_offset_ + ei * sizeof(RouteEdge),
                               reinterpret_cast<uint8_t*>(&out_edge), sizeof(RouteEdge)) == sizeof(RouteEdge);
}

/**
 * @brief Find the nearest node to the given coordinates.
 *
 * Only searches cells within ±1.5° of the target to avoid a full scan.
 *
 * @param lat Latitude in degrees
 * @param lon Longitude in degrees
 * @return Global node index of the nearest node
 */
uint32_t GraphLoader::nearestNode(float lat, float lon) const
{
    uint32_t best_i  = 0;
    float    best_d  = 1e30f;
    float    cos_lat = cosf(lat * 3.14159265f / 180.f);

    for (const auto& cell : cellIndex_)
    {
        if (cell.lat_floor > (int16_t)(lat + 1.5f) || cell.lat_floor < (int16_t)(lat - 1.5f)) continue;
        if (cell.lon_floor > (int16_t)(lon + 1.5f) || cell.lon_floor < (int16_t)(lon - 1.5f)) continue;

        bool fully_cached = nodeCache_count_ > 0
                         && cell.node_offset >= nodeCache_base_
                         && (cell.node_offset + cell.node_count) <= (nodeCache_base_ + nodeCache_count_);

        if (fully_cached)
        {
            for (uint32_t j = 0; j < cell.node_count; ++j)
            {
                const RouteNode& n = nodeCache_[cell.node_offset + j - nodeCache_base_];
                float dlat = n.lat - lat;
                float dlon = (n.lon - lon) * cos_lat;
                float d    = dlat * dlat + dlon * dlon;
                if (d < best_d) { best_d = d; best_i = cell.node_offset + j; }
            }
        }
        else if (file_)
        {
            // Cell not in PSRAM cache — read node-by-node from SD (no heap alloc)
            RouteNode n;
            for (uint32_t j = 0; j < cell.node_count; ++j)
            {
                long off = (long)(nodes_base_offset_ + (cell.node_offset + j) * sizeof(RouteNode));
                if (storage.seekAndRead(file_, off, reinterpret_cast<uint8_t*>(&n), sizeof(RouteNode)) != sizeof(RouteNode))
                    continue;
                float dlat = n.lat - lat;
                float dlon = (n.lon - lon) * cos_lat;
                float d    = dlat * dlat + dlon * dlon;
                if (d < best_d) { best_d = d; best_i = cell.node_offset + j; }
            }
        }
    }
    printf("DEBUG GL: nearestNode(%.5f,%.5f) -> gi=%lu\n", lat, lon, (unsigned long)best_i);
    return best_i;
}

/**
 * @brief Resolve the global edge range [e_start, e_end) for a node.
 *
 * RouteNode::edge_offset is relative to the cell's edge block, so
 * the global index is cell.edge_offset + node.edge_offset.
 *
 * @param gi     Global node index
 * @param e_start Output: first global edge index
 * @param e_end   Output: one-past-last global edge index
 */
void GraphLoader::edgeForNode(uint32_t gi, uint32_t& e_start, uint32_t& e_end) const
{
    for (const auto& cell : cellIndex_)
    {
        if (gi < cell.node_offset || gi >= cell.node_offset + cell.node_count) continue;

        RouteNode n;
        if (!getNode(gi, n)) { e_start = 0; e_end = 0; return; }

        e_start = cell.edge_offset + n.edge_offset;

        uint32_t local_idx = gi - cell.node_offset;
        if (local_idx + 1 < cell.node_count)
        {
            RouteNode next;
            if (getNode(gi + 1, next))
                e_end = cell.edge_offset + next.edge_offset;
            else
                e_end = e_start;
        }
        else
        {
            e_end = cell.edge_offset + cell.edge_count;
        }
        return;
    }
    e_start = 0; e_end = 0;
}

/**
 * @brief Cache edges for nodes within the src↔dst bbox + margin in PSRAM.
 *
 * Scans the nodeCache to find the contiguous edge range covering all nodes
 * that fall inside the bounding box. Loads that range with a single read.
 * Falls back silently if PSRAM is insufficient — getEdgesForNode will hit SD.
 *
 * @param src_lat Source latitude
 * @param src_lon Source longitude
 * @param dst_lat Destination latitude
 * @param dst_lon Destination longitude
 * @param margin  Degrees of padding around the bbox (default 0.05° ≈ 5.5 km)
 * @return true if the edge cache was loaded successfully
 */
bool GraphLoader::buildEdgeCache(float src_lat, float src_lon,
                                 float dst_lat, float dst_lon, float margin)
{
    if (nodeCache_count_ == 0 || !file_) return false;

    float lat_min = fminf(src_lat, dst_lat) - margin;
    float lat_max = fmaxf(src_lat, dst_lat) + margin;
    float lon_min = fminf(src_lon, dst_lon) - margin;
    float lon_max = fmaxf(src_lon, dst_lon) + margin;

    for (const auto& cell : cellIndex_)
    {
        if (cell.node_offset < nodeCache_base_ ||
            cell.node_offset + cell.node_count > nodeCache_base_ + nodeCache_count_) continue;

        uint32_t e_first = UINT32_MAX;
        uint32_t e_last  = 0;

        for (uint32_t j = 0; j < cell.node_count; ++j)
        {
            const RouteNode& n = nodeCache_[cell.node_offset + j - nodeCache_base_];
            if (n.lat < lat_min || n.lat > lat_max) continue;
            if (n.lon < lon_min || n.lon > lon_max) continue;

            uint32_t ge_start = cell.edge_offset + n.edge_offset;
            if (ge_start < e_first) e_first = ge_start;

            uint32_t ge_end;
            if (j + 1 < cell.node_count)
            {
                const RouteNode& next = nodeCache_[cell.node_offset + j + 1 - nodeCache_base_];
                ge_end = cell.edge_offset + next.edge_offset;
            }
            else
                ge_end = cell.edge_offset + cell.edge_count;
            if (ge_end > e_last) e_last = ge_end;
        }

        if (e_first >= e_last) continue;

        uint32_t count  = e_last - e_first;
        size_t   needed = (size_t)count * sizeof(RouteEdge);
        size_t   avail  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        printf("DEBUG GL: edge bbox cache %lu edges (%lu KB), PSRAM free=%lu KB\n",
               (unsigned long)count, (unsigned long)(needed / 1024),
               (unsigned long)(avail / 1024));

        if (needed + (256 * 1024) > avail)
        {
            printf("DEBUG GL: edge cache skip (not enough PSRAM)\n");
            return false;
        }

        try { edgeCache_.resize(count); }
        catch (const std::bad_alloc&)
        {
            edgeCache_.clear();
            printf("DEBUG GL: edge cache bad_alloc\n");
            return false;
        }

        storage.seekAndRead(file_,
                            edges_base_offset_ + e_first * sizeof(RouteEdge),
                            reinterpret_cast<uint8_t*>(edgeCache_.data()), needed);
        edgeCache_base_  = e_first;
        edgeCache_count_ = count;
        printf("DEBUG GL: edgeCache OK — %lu edges (base=%lu)\n",
               (unsigned long)count, (unsigned long)e_first);
        return true;
    }
    return false;
}

/**
 * @brief Read all edges for a node — from PSRAM edge cache if available, otherwise from SD.
 *
 * @param gi    Global node index
 * @param buf   Caller-supplied buffer (must fit MAX_EDGES_PER_NODE entries)
 * @param count Output: number of edges read
 * @return true on success (count may be 0 for leaf nodes)
 */
bool GraphLoader::getEdgesForNode(uint32_t gi, RouteEdge* buf, uint32_t& count) const
{
    uint32_t e_start, e_end;
    edgeForNode(gi, e_start, e_end);
    if (e_end <= e_start) { count = 0; return true; }
    count = e_end - e_start;
    if (count > MAX_EDGES_PER_NODE_GL) { count = 0; return true; }

    if (edgeCache_count_ > 0 &&
        e_start >= edgeCache_base_ &&
        e_end   <= edgeCache_base_ + edgeCache_count_)
    {
        memcpy(buf, &edgeCache_[e_start - edgeCache_base_], count * sizeof(RouteEdge));
        return true;
    }

    if (!file_) return false;
    size_t bytes = count * sizeof(RouteEdge);
    return storage.seekAndRead(file_,
                               edges_base_offset_ + e_start * sizeof(RouteEdge),
                               reinterpret_cast<uint8_t*>(buf),
                               bytes) == bytes;
}

/**
 * @brief Unload graph data, close FILE*, and free PSRAM cache.
 */
void GraphLoader::unload()
{
    if (file_) { storage.close(file_); file_ = nullptr; }
    cellIndex_.clear();
    nodeCache_.clear();
    nodeCache_base_  = 0;
    nodeCache_count_ = 0;
    edgeCache_.clear();
    edgeCache_base_  = 0;
    edgeCache_count_ = 0;
    totalNodes_      = 0;
    loaded_          = false;
}
