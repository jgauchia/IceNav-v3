/**
 * @file astar.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  A* routing algorithm implementation
 * @version 0.2.9
 * @date 2026-06
 */

#include "astar.hpp"
#include <queue>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>
#include <unordered_map>
#include "PsramAllocator.hpp"
#include "graph_loader.hpp"
struct AStarState
{
    uint32_t f;
    uint32_t g;
    uint32_t node;
    bool operator>(const AStarState& o) const { return f > o.f; }
};

/**
 * @brief Admissible heuristic: straight-line travel time at maximum road speed.
 *
 * @param node  Current global node index
 * @param dst   Destination global node index
 * @param graph Loaded graph
 * @return Estimated cost in tenths of second
 */
// Weight > 1 makes the heuristic inadmissible but drastically reduces node expansions.
// In urban graphs (actual speed ~40 km/h vs assumed 130 km/h), the unweighted h
// underestimates by ~3x, causing near-Dijkstra behavior. 1.5x keeps route quality
// within ~5% of optimal on real road networks.
static constexpr float ASTAR_WEIGHT       = 1.5f;    /**< Heuristic inflation factor — trades optimality for speed. */
static constexpr float METERS_PER_DEGREE  = 111319.0f; /**< Approximate metres per degree of latitude at the equator. */

/**
* @brief Calculates the heuristic estimated cost between a node and the destination.
*
* @param node        ID of the current node to evaluate.
* @param dst_cached  Cached destination node data for performance.
* @param cos_dst_lat Pre-calculated cosine of destination latitude for longitude compensation.
* @param graph       GraphLoader reference to access node coordinates.
* @param maxSpeedMs  Maximum speed in m/s for the active routing profile.
* @return Estimated cost to reach the destination, scaled by ASTAR_WEIGHT.
*/
static uint32_t heuristic(uint32_t node, const RouteNode& dst_cached, float cos_dst_lat,
                          const GraphLoader& graph, float maxSpeedMs)
{
    RouteNode a;
    if (!graph.getNode(node, a))
        return 0;
    float dlat = dst_cached.lat - a.lat;
    float dlon  = (dst_cached.lon - a.lon) * cos_dst_lat;
    float dist  = sqrtf(dlat * dlat + dlon * dlon) * METERS_PER_DEGREE;
    return (uint32_t)(dist / maxSpeedMs * 10.f * ASTAR_WEIGHT);
}

/**
 * @brief Compute an A* route between two global node indices.
 *
 * @param graph        Loaded graph (GraphLoader::load() must have succeeded)
 * @param src_node     Global index of the source node
 * @param dst_node     Global index of the destination node
 * @param maxSpeedKmh  Maximum speed in km/h for the routing profile (car=130, bike=25, pedestrian=5)
 * @return TrackVector with route waypoints, or empty if no path found
 */
TrackVector astarRoute(const GraphLoader& graph, uint32_t src_node, uint32_t dst_node, float maxSpeedKmh)
{
    float maxSpeedMs = maxSpeedKmh / 3.6f;
    const uint32_t INF = UINT32_MAX;

    TrackVector result;

    RouteNode dst_cached;
    if (!graph.getNode(dst_node, dst_cached))
        return result;
    float cos_dst_lat = cosf(dst_cached.lat * 3.14159265f / 180.f);

    using U32U32Map = std::unordered_map<uint32_t, uint32_t,
        std::hash<uint32_t>, std::equal_to<uint32_t>,
        PsramAllocator<std::pair<const uint32_t, uint32_t>>>;

    U32U32Map g_cost;
    U32U32Map prev;

    // Pre-reserve to avoid rehashing under PSRAM pressure (~10-14 rehashings otherwise)
    g_cost.reserve(30000);
    prev.reserve(30000);

    using PQ = std::priority_queue<AStarState,
                                   std::vector<AStarState, PsramAllocator<AStarState>>,
                                   std::greater<AStarState>>;
    PQ pq;

    g_cost[src_node] = 0;
    pq.push({heuristic(src_node, dst_cached, cos_dst_lat, graph, maxSpeedMs), 0u, src_node});

    while (!pq.empty())
    {
        AStarState top = pq.top(); pq.pop();
        uint32_t u = top.node;

        // Lazy deletion: discard stale PQ entries instead of a separate visited set.
        // Saves ~180 KB PSRAM and reduces max PQ size from O(E) to O(V).
        auto it = g_cost.find(u);
        if (it != g_cost.end() && top.g > it->second)
            continue;
        if (u == dst_node)
            break;

        RouteEdge edge_buf[MAX_EDGES_PER_NODE_GL];
        uint32_t edge_count = 0;
        if (!graph.getEdgesForNode(u, edge_buf, edge_count))
            continue;

        auto g_it = g_cost.find(u);
        uint32_t current_g = (g_it != g_cost.end()) ? g_it->second : INF;

        for (uint32_t ei = 0; ei < edge_count; ++ei)
        {
            const RouteEdge& e = edge_buf[ei];
            uint32_t ng = current_g + e.cost;
            auto nb_it = g_cost.find(e.dst_node);
            uint32_t neighbor_g = (nb_it != g_cost.end()) ? nb_it->second : INF;

            if (ng < neighbor_g)
            {
                g_cost[e.dst_node] = ng;
                prev[e.dst_node]   = u;
                uint32_t h = heuristic(e.dst_node, dst_cached, cos_dst_lat, graph, maxSpeedMs);
                pq.push({ng + h, ng, e.dst_node});
            }
        }
    }

    if (g_cost.find(dst_node) == g_cost.end())
        return result;

    uint32_t cur = dst_node;
    while (cur != UINT32_MAX)
    {
        RouteNode n;
        if (!graph.getNode(cur, n))
            break;
        wayPoint wp{};
        wp.lat = n.lat;
        wp.lon = n.lon;
        result.push_back(wp);
        auto p_it = prev.find(cur);
        cur = (p_it != prev.end()) ? p_it->second : UINT32_MAX;
    }
    std::reverse(result.begin(), result.end());
    return result;
}
