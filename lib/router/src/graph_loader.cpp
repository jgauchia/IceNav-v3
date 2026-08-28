/**
 * @file graph_loader.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ROUTE.bin paged graph loader with on-demand PSRAM cache and turn-restriction support
 * @version 0.3.0
 * @date 2026-06
 */

#include "graph_loader.hpp"
#include "storage.hpp"
#include "esp_log.h"
#include "settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "esp_heap_caps.h"

extern Storage storage;

/**
 * @brief Load the cell index from ROUTE.bin.
 *
 * Only the index is held in internal RAM. Cell node/edge data is loaded on-demand
 * into a PSRAM page cache (PAGE_CACHE_MAX entries, LRU eviction).
 *
 * @return true on success, false if file missing or format error
 */
bool GraphLoader::load()
{
    unload();

    FILE* f = storage.open(routeBinPath(navSet.routeSpeed), "rb");
    if (!f)
        return false;

    RouteFileHeader hdr;
    if (storage.read(f, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) != sizeof(hdr))
    {
        storage.close(f);
        return false;
    }

    if (memcmp(hdr.magic, ROUTE_MAGIC, 4) != 0)
    {
        storage.close(f);
        return false;
    }

    cellIndex.resize(hdr.cell_count);
    size_t indexBytes = hdr.cell_count * sizeof(CellIndexEntry);
    if (storage.read(f, reinterpret_cast<uint8_t*>(cellIndex.data()), indexBytes) != indexBytes)
    {
        ESP_LOGE("GraphLoader", "Partial read of cell index (%u cells)", hdr.cell_count);
        storage.close(f);
        return false;
    }

    // Data block starts immediately after header + index.
    data_base_offset = sizeof(RouteFileHeader) + hdr.cell_count * sizeof(CellIndexEntry);

    nodeCount = 0;
    for (const auto& c : cellIndex)
    {
        uint32_t end = c.node_offset + c.node_count;
        if (end > nodeCount)
            nodeCount = end;
    }

    // Global edge base per cell (cumulative edge_count) — used to map a cell-local
    // edge to its global index for turn restrictions.
    cellEdgeBase.assign(hdr.cell_count, 0);
    uint32_t edge_acc = 0;
    for (uint32_t i = 0; i < hdr.cell_count; ++i)
    {
        cellEdgeBase[i] = edge_acc;
        edge_acc += cellIndex[i].edge_count;
    }

    // Read the turn-restriction table appended after the data block.
    turnRestrictions.clear();
    if (hdr.turn_count > 0)
    {
        turnRestrictions.resize(hdr.turn_count);
        size_t table_bytes = hdr.turn_count * sizeof(TurnRestriction);
        // Data block size = sum over cells of (nodes*sizeof + edges*sizeof);
        // seek and read exactly at data_base + total_data_size.
        uint32_t total_data = 0;
        for (uint32_t i = 0; i < hdr.cell_count; ++i)
            total_data += cellIndex[i].node_count * sizeof(RouteNode)
                        + cellIndex[i].edge_count * sizeof(RouteEdge);
        if (storage.seekAndRead(f, data_base_offset + total_data,
                                reinterpret_cast<uint8_t*>(turnRestrictions.data()),
                                table_bytes) != table_bytes)
        {
            ESP_LOGE("GraphLoader", "Partial read of turn restriction table");
            turnRestrictions.clear();
        }
    }

    // Sort by via_node so isTurnForbidden can binary-search a small range per node
    // instead of scanning the whole table for every expanded edge.
    std::sort(turnRestrictions.begin(), turnRestrictions.end(),
              [](const TurnRestriction& a, const TurnRestriction& b)
              { return a.via_node < b.via_node; });

    file   = f;
    loaded = true;

    return true;
}

/**
 * @brief Absolute global edge index for a cell-local edge offset.
 */
uint32_t GraphLoader::edgeGlobalOffset(uint32_t cell_idx, uint32_t rel_edge) const
{
    if (cell_idx >= (uint32_t)cellEdgeBase.size())
        return UINT32_MAX;
    return cellEdgeBase[cell_idx] + rel_edge;
}

/**
 * @brief Global edge index for a node-local edge offset.
 *
 * Resolves the owning cell of global node gi, then adds the local edge offset.
 */
uint32_t GraphLoader::edgeGlobalForNode(uint32_t gi, uint32_t rel_edge) const
{
    uint32_t ci = cellForNode(gi);
    if (ci == UINT32_MAX)
        return UINT32_MAX;
    return edgeGlobalOffset(ci, rel_edge);
}

/**
 * @brief Check whether the turn (in_edge -> out_edge) through via_node is forbidden.
 *
 * The table is sorted by via_node at load time; only the small range of entries
 * sharing via_node is scanned (typically 1-2 entries per intersection).
 */
bool GraphLoader::isTurnForbidden(uint32_t via_node, uint32_t in_edge, uint32_t out_edge) const
{
    auto it = std::lower_bound(turnRestrictions.begin(), turnRestrictions.end(), via_node,
                               [](const TurnRestriction& tr, uint32_t v)
                               { return tr.via_node < v; });
    for (; it != turnRestrictions.end() && it->via_node == via_node; ++it)
    {
        if (it->in_edge == in_edge && it->out_edge == out_edge)
            return true;
    }
    return false;
}

/**
 * @brief Return the cellIndex index for the cell owning global node gi.
 *
 * Uses binary search on node_offset (cellIndex is sorted ascending by it).
 */
uint32_t GraphLoader::cellForNode(uint32_t gi) const
{
    if (cellIndex.empty())
        return UINT32_MAX;

    // Find the last cell whose node_offset <= gi.
    uint32_t lo = 0;
    uint32_t hi = (uint32_t)cellIndex.size() - 1;
    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo + 1) / 2;
        if (cellIndex[mid].node_offset <= gi)
            lo = mid;
        else
            hi = mid - 1;
    }

    const CellIndexEntry& c = cellIndex[lo];
    return (gi >= c.node_offset && gi < c.node_offset + c.node_count) ? lo : UINT32_MAX;
}

/**
 * @brief Evict the page with the lowest LRU stamp from pageCache.
 */
void GraphLoader::evictLRU() const
{
    if (pageCache.empty())
        return;

    uint32_t oldest_key   = 0;
    uint32_t oldest_stamp = UINT32_MAX;

    for (const auto& kv : pageCache)
    {
        if (kv.second.lru_stamp < oldest_stamp)
        {
            oldest_stamp = kv.second.lru_stamp;
            oldest_key   = kv.first;
        }
    }
    pageCache.erase(oldest_key);
}

/**
 * @brief Load a cell's nodes and edges into the page cache.
 *
 * Evicts the LRU page if the cache is full. Returns nullptr if the load fails
 * or there is insufficient PSRAM.
 *
 * @param cell_idx Index into cellIndex
 * @return Pointer to the loaded PageData, or nullptr on failure
 */
GraphLoader::PageData* GraphLoader::fetchPage(uint32_t cell_idx) const
{
    auto it = pageCache.find(cell_idx);
    if (it != pageCache.end())
    {
        it->second.lru_stamp = ++lru_clock;
        return &it->second;
    }

    if (!file)
        return nullptr;

    const CellIndexEntry& c = cellIndex[cell_idx];
    size_t node_bytes = c.node_count * sizeof(RouteNode);
    size_t edge_bytes = c.edge_count * sizeof(RouteEdge);
    size_t needed     = node_bytes + edge_bytes;
    size_t avail      = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    if (pageCache.size() >= PAGE_CACHE_MAX)
        evictLRU();

    // Re-check PSRAM after potential eviction
    avail = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (needed + (64 * 1024) > avail)
        return nullptr;

    PageData page;
    page.cell_idx  = cell_idx;
    page.lru_stamp = ++lru_clock;

    try
    {
        page.nodes.resize(c.node_count);
        page.edges.resize(c.edge_count);
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }

    uint32_t file_offset = data_base_offset + c.data_offset;
    bool ok = true;
    if (c.node_count > 0)
        ok = storage.seekAndRead(file, file_offset,
                            reinterpret_cast<uint8_t*>(page.nodes.data()), node_bytes) && ok;

    if (c.edge_count > 0)
        ok = storage.seekAndRead(file, file_offset + node_bytes,
                            reinterpret_cast<uint8_t*>(page.edges.data()), edge_bytes) && ok;

    auto res = pageCache.emplace(cell_idx, std::move(page));
    if (!res.second)
        return nullptr;
    return &res.first->second;
}

/**
 * @brief Preload the single cell that contains the given lat/lon point.
 *
 * Used before nearestNode() to ensure the cell is in PSRAM cache.
 *
 * @param lat Latitude in degrees
 * @param lon Longitude in degrees
 */
void GraphLoader::preloadPoint(float lat, float lon) const
{
    int32_t lat_e4 = (int32_t)(floorf(lat * 10000.f / 500.f) * 500.f);
    int32_t lon_e4 = (int32_t)(floorf(lon * 10000.f / 500.f) * 500.f);

    for (uint32_t i = 0; i < (uint32_t)cellIndex.size(); ++i)
    {
        if (cellIndex[i].lat_e4 == lat_e4 && cellIndex[i].lon_e4 == lon_e4)
        {
            fetchPage(i);
            return;
        }
    }
}

/**
 * @brief Read a single node — from page cache if available, otherwise from SD.
 *
 * @param gi Global node index
 * @param out_node Output node
 * @return true if successful
 */
bool GraphLoader::getNode(uint32_t gi, RouteNode& out_node) const
{
    uint32_t ci = cellForNode(gi);
    if (ci == UINT32_MAX)
        return false;

    PageData* page = fetchPage(ci);
    if (page)
    {
        uint32_t local = gi - cellIndex[ci].node_offset;
        out_node = page->nodes[local];
        return true;
    }

    // Fallback: read directly from SD using the cell's data_offset.
    if (!file)
        return false;
    const CellIndexEntry& cb = cellIndex[ci];
    uint32_t local = gi - cb.node_offset;
    uint32_t file_off = data_base_offset + cb.data_offset + local * sizeof(RouteNode);
    return storage.seekAndRead(file, file_off,
                               reinterpret_cast<uint8_t*>(&out_node),
                               sizeof(RouteNode)) == sizeof(RouteNode);
}

/**
 * @brief Read a single node's absolute coordinates (rebuilt from int16 cell offsets).
 *
 * @param gi    Global node index
 * @param lat   Output latitude in degrees
 * @param lon   Output longitude in degrees
 * @return true if successful
 */
bool GraphLoader::getNodeCoords(uint32_t gi, float& lat, float& lon) const
{
    uint32_t ci = cellForNode(gi);
    if (ci == UINT32_MAX)
        return false;

    RouteNode n;
    if (!getNode(gi, n))
        return false;

    const CellIndexEntry& c = cellIndex[ci];
    lat = nodeLatDeg(c.lat_e4, n.lat_off);
    lon = nodeLonDeg(c.lon_e4, n.lon_off);
    return true;
}

/**
 * @brief Find the nearest graph node to the given coordinates.
 *
 * Searches only pages already in the PSRAM cache to avoid SD I/O and cache
 * eviction. Tries ±0.3° first; expands to ±1.0° if no cached page is found.
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

    // Two-pass: tight radius first, wider only if nothing found in cache.
    static constexpr float RADII[2] = { 0.3f, 1.0f };

    for (float radius : RADII)
    {
        for (uint32_t ci = 0; ci < (uint32_t)cellIndex.size(); ++ci)
        {
            const CellIndexEntry& cell = cellIndex[ci];
            float cell_lat = cell.lat_e4 / 10000.0f;
            float cell_lon = cell.lon_e4 / 10000.0f;

            if (cell_lat > lat + radius || cell_lat + 0.1f < lat - radius)
                continue;
            if (cell_lon > lon + radius || cell_lon + 0.1f < lon - radius)
                continue;

            // Only search pages already in PSRAM — never trigger an SD load here.
            auto it = pageCache.find(ci);
            if (it == pageCache.end())
                continue;
            const PageData& page = it->second;

            // Cell centre (degrees), precomputed once per cell.
            const float cell_center_lat = (cell.lat_e4 + 250) / 10000.0f;
            const float cell_center_lon = (cell.lon_e4 + 250) / 10000.0f;
            constexpr float STEP_DEG = 0.05f / 65536.0f;

            for (uint32_t j = 0; j < cell.node_count; ++j)
            {
                const RouteNode& n = page.nodes[j];
                float dlat = cell_center_lat + n.lat_off * STEP_DEG - lat;
                float dlon = (cell_center_lon + n.lon_off * STEP_DEG - lon) * cos_lat;
                float d    = dlat * dlat + dlon * dlon;
                if (d < best_d) { best_d = d; best_i = cell.node_offset + j; }
            }
        }

        if (best_d < 1e30f) break;  // found something — skip wider pass
    }

    return best_i;
}

/**
 * @brief Read all edges for a node — from page cache if available, otherwise from SD.
 *
 * @param gi    Global node index
 * @param buf   Caller-supplied buffer (must fit MAX_EDGES_PER_NODE_GL entries)
 * @param count Output: number of edges read
 * @return true on success (count may be 0 for leaf nodes)
 */
bool GraphLoader::getEdgesForNode(uint32_t gi, RouteEdge* buf, uint32_t& count) const
{
    uint32_t ci = cellForNode(gi);
    if (ci == UINT32_MAX) { count = 0; return true; }

    const CellIndexEntry& cell  = cellIndex[ci];
    uint32_t local_idx          = gi - cell.node_offset;

    PageData* page = fetchPage(ci);

    uint32_t rel_e_start;
    uint32_t rel_e_end;

    if (page)
    {
        rel_e_start = page->nodes[local_idx].edge_offset;
        if (local_idx + 1 < cell.node_count)
            rel_e_end = page->nodes[local_idx + 1].edge_offset;
        else
            rel_e_end = cell.edge_count;

        count = rel_e_end - rel_e_start;
        if (count > MAX_EDGES_PER_NODE_GL) { count = 0; return true; }

        memcpy(buf, &page->edges[rel_e_start], count * sizeof(RouteEdge));
        return true;
    }

    // Fallback: read from SD without page cache
    if (!file) { count = 0; return false; }

    RouteNode n, nxt;
    if (!getNode(gi, n)) { count = 0; return false; }
    rel_e_start = n.edge_offset;

    if (local_idx + 1 < cell.node_count)
    {
        if (getNode(gi + 1, nxt))
            rel_e_end = nxt.edge_offset;
        else
            rel_e_end = rel_e_start;
    }
    else
        rel_e_end = cell.edge_count;

    count = rel_e_end - rel_e_start;
    if (count > MAX_EDGES_PER_NODE_GL) { count = 0; return true; }

    // Edge block starts after the node block within this cell's data_offset.
    size_t bytes = count * sizeof(RouteEdge);
    uint32_t edges_start = data_base_offset + cell.data_offset
                         + cell.node_count * sizeof(RouteNode)
                         + rel_e_start * sizeof(RouteEdge);
    return storage.seekAndRead(file, edges_start,
                               reinterpret_cast<uint8_t*>(buf),
                               bytes) == bytes;
}

/**
 * @brief Unload all graph data, close FILE*, and free PSRAM page cache.
 */
void GraphLoader::unload()
{
    if (file) { storage.close(file); file = nullptr; }
    cellIndex.clear();
    turnRestrictions.clear();
    cellEdgeBase.clear();
    pageCache.clear();
    lru_clock         = 0;
    data_base_offset  = 0;
    nodeCount        = 0;
    loaded            = false;
}
