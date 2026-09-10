/**
 * @file astar.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  A* routing algorithm implementation with turn restrictions.
 * @version 0.3.0
 * @date 2026-06
 */

#include "astar.hpp"
#include <queue>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>
#include "PsramAllocator.hpp"
#include "graph_loader.hpp"

// Weight > 1 makes the heuristic inadmissible but drastically reduces node expansions.
// In urban graphs (actual speed ~40 km/h vs assumed 130 km/h), the unweighted h
// underestimates by ~3x, causing near-Dijkstra behavior. 1.5x keeps route quality
// within ~5% of optimal on real road networks. Not applied to the WALK profile:
// its h already reaches the real walking speed, so inflating it overestimates and
// the search terminates on a sub-optimal zig-zag route.
static constexpr float ASTAR_WEIGHT       = 1.5f;    /**< Heuristic inflation factor — trades optimality for speed. */
static constexpr float METERS_PER_DEGREE  = 111319.0f; /**< Approximate metres per degree of latitude at the equator. */

// Turn-penalty: when the path makes a turn sharper than 45 deg at a junction,
// add a fixed time cost (TURN_PENALTY_TENTHS). This keeps routes on the straight
// course in grids where several outgoing edges have equal cost (the WALK/BIKE
// profiles with flat speed tables): zig-zagging across both sidewalks through
// zebra crossings becomes more expensive than staying on one side.
// cos(45 deg): a turn is "sharp" when the cosine of the angle between the
// incoming and outgoing directions drops below this value (no acosf needed).
static constexpr float    TURN_MIN_ANGLE_COS    = 0.70710678f;
static constexpr uint32_t TURN_PENALTY_TENTHS   = 30u;   /**< 3 s extra per sharp turn (in tenths of second). */

// Sentinel: state that represents "no incoming edge" (route origin).
static constexpr uint32_t EDGE_NONE = UINT32_MAX;

// Sentinel: empty slot in the state table. Never a valid state key: a real key
// requires node < 2^32 and edge_in < 2^32, so the all-ones value is free.
static constexpr uint64_t STATE_EMPTY = UINT64_MAX;

// State key: (node << 32) | edge_in. Two states with the same node but different
// incoming edge are distinct, which is required to enforce turn restrictions.
static inline uint64_t stateKey(uint32_t node, uint32_t edge_in)
{
    return ((uint64_t)node << 32) | edge_in;
}

struct AStarState
{
    uint32_t f;
    uint32_t g;
    uint32_t node;
    uint32_t edge_in;   // global edge index we arrived on; EDGE_NONE at source
    bool operator>(const AStarState& o) const { return f > o.f; }
};

// splitmix64 finalizer: distributes state keys (node << 32) | edge_in over the
// power-of-two slot range (the raw low bits alone would cluster badly).
static uint64_t mix64(uint64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

/**
 * @brief Flat open-addressing hash table for A* states, backed by PSRAM.
 *
 * State key: (node << 32) | edge_in (see stateKey). Each state keeps its g
 * cost, the key of its parent state (path reconstruction and turn-angle
 * checks) and an expanded flag in four parallel contiguous arrays. Linear
 * probing, doubling when the load factor reaches 0.6. One structure replaces
 * the three per-state hash containers: no per-element heap allocation and a
 * single probe per lookup instead of a bucket-chain walk.
 */
class StateTable
{
public:
    explicit StateTable(uint32_t initial_capacity)
    {
        rehash(initial_capacity);
    }

    // Slot index for key, or UINT32_MAX when absent.
    uint32_t findSlot(uint64_t key) const
    {
        uint32_t idx = (uint32_t)mix64(key) & mask;
        while (keys[idx] != STATE_EMPTY)
        {
            if (keys[idx] == key)
                return idx;
            idx = (idx + 1) & mask;
        }
        return UINT32_MAX;
    }

    uint32_t gAt(uint32_t slot) const        { return gs[slot]; }
    uint64_t parentAt(uint32_t slot) const   { return parents[slot]; }
    bool     expandedAt(uint32_t slot) const { return expd[slot] != 0; }
    void     markExpandedAt(uint32_t slot)   { expd[slot] = 1; }

    // g cost of key, or UINT32_MAX (INF) when absent.
    uint32_t getG(uint64_t key) const
    {
        uint32_t slot = findSlot(key);
        return (slot != UINT32_MAX) ? gs[slot] : UINT32_MAX;
    }

    // Parent state key of key, or UINT64_MAX when absent.
    uint64_t getPrev(uint64_t key) const
    {
        uint32_t slot = findSlot(key);
        return (slot != UINT32_MAX) ? parents[slot] : UINT64_MAX;
    }

    // Insert key or, when already present, update its g cost and parent key.
    void upsert(uint64_t key, uint32_t g, uint64_t parent)
    {
        if (count * 5 >= (mask + 1) * 3)
            rehash((mask + 1) * 2);
        uint32_t idx = (uint32_t)mix64(key) & mask;
        while (keys[idx] != STATE_EMPTY && keys[idx] != key)
            idx = (idx + 1) & mask;
        if (keys[idx] == STATE_EMPTY)
        {
            keys[idx] = key;
            ++count;
        }
        gs[idx]      = g;
        parents[idx] = parent;
    }

private:
    // Allocate a power-of-two table of at least new_capacity slots and rehash
    // the occupied entries into it.
    void rehash(uint32_t new_capacity)
    {
        uint32_t cap = 1;
        while (cap < new_capacity)
            cap <<= 1;

        std::vector<uint64_t, PsramAllocator<uint64_t>> old_keys;
        std::vector<uint32_t, PsramAllocator<uint32_t>> old_gs;
        std::vector<uint8_t, PsramAllocator<uint8_t>>   old_expd;
        std::vector<uint64_t, PsramAllocator<uint64_t>> old_parents;
        old_keys.swap(keys);
        old_gs.swap(gs);
        old_expd.swap(expd);
        old_parents.swap(parents);

        mask  = cap - 1;
        count = 0;
        keys.resize(cap, STATE_EMPTY);
        gs.resize(cap);
        expd.assign(cap, 0);
        parents.resize(cap);

        for (uint32_t i = 0; i < old_keys.size(); ++i)
        {
            if (old_keys[i] == STATE_EMPTY)
                continue;
            uint32_t idx = (uint32_t)mix64(old_keys[i]) & mask;
            while (keys[idx] != STATE_EMPTY)
                idx = (idx + 1) & mask;
            keys[idx]    = old_keys[i];
            gs[idx]      = old_gs[i];
            expd[idx]    = old_expd[i];
            parents[idx] = old_parents[i];
            ++count;
        }
    }

    std::vector<uint64_t, PsramAllocator<uint64_t>> keys;
    std::vector<uint32_t, PsramAllocator<uint32_t>> gs;
    std::vector<uint8_t, PsramAllocator<uint8_t>>   expd;
    std::vector<uint64_t, PsramAllocator<uint64_t>> parents;
    uint32_t mask  = 0;
    uint32_t count = 0;
};

/**
 * @brief Admissible heuristic: straight-line travel time at maximum road speed.
 *
 * @param node        Current global node index
 * @param dst_lat     Destination latitude in degrees
 * @param dst_lon     Destination longitude in degrees
 * @param cos_dst_lat Cosine of the destination latitude (for longitude compensation)
 * @param graph       Loaded graph
 * @param maxSpeedMs  Maximum speed in m/s for the active profile
 * @return Estimated cost in tenths of second, scaled by ASTAR_WEIGHT
 */
static uint32_t heuristic(uint32_t node, float dst_lat, float dst_lon, float cos_dst_lat,
                          const GraphLoader& graph, float maxSpeedMs)
{
    float alat, alon;
    if (!graph.getNodeCoords(node, alat, alon))
        return 0;
    float dlat  = dst_lat - alat;
    float dlon  = (dst_lon - alon) * cos_dst_lat;
    float dist  = sqrtf(dlat * dlat + dlon * dlon) * METERS_PER_DEGREE;
    // WALK (<=10 km/h): h at the profile speed is already tight, keep it admissible.
    float weight = (maxSpeedMs < 3.0f) ? 1.0f : ASTAR_WEIGHT;
    return (uint32_t)(dist / maxSpeedMs * 10.f * weight);
}

/**
 * @brief Compute an A* route between two global node indices.
 *
 * Uses state (node, incoming edge) so turn restrictions and a turn penalty can
 * be honoured. The cost of an outgoing edge `e` from state `u` is increased by
 * the fixed TURN_PENALTY_TENTHS when the angle between the incoming and outgoing
 * edges exceeds 45 degrees (checked via dot < cos(45 deg), no acosf).
 *
 * @param graph        Loaded graph (GraphLoader::load() must have succeeded)
 * @param src_node     Global index of the source node
 * @param dst_node     Global index of the destination node
 * @param maxSpeedKmh  Maximum speed in km/h for the active profile
 * @return TrackVector with route waypoints, or empty if no path found
 */
TrackVector astarRoute(const GraphLoader& graph, uint32_t src_node, uint32_t dst_node, float maxSpeedKmh)
{
    float maxSpeedMs = maxSpeedKmh / 3.6f;
    const uint32_t INF = UINT32_MAX;

    TrackVector result;

    float dst_lat, dst_lon;
    if (!graph.getNodeCoords(dst_node, dst_lat, dst_lon))
        return result;
    float cos_dst_lat = cosf(dst_lat * 3.14159265f / 180.f);

    // One flat PSRAM table holds g cost, parent and expanded flag for every
    // state (see StateTable). Contiguous storage: no per-element heap
    // allocation and one probe per lookup.
    StateTable states(1u << 14);

    using PqStorage = std::vector<AStarState, PsramAllocator<AStarState>>;
    using PQ = std::priority_queue<AStarState, PqStorage, std::greater<AStarState>>;
    PqStorage pq_storage;
    pq_storage.reserve(30000);
    PQ pq(std::greater<AStarState>(), std::move(pq_storage));

    uint64_t src_key = stateKey(src_node, EDGE_NONE);
    states.upsert(src_key, 0u, UINT64_MAX);
    pq.push({heuristic(src_node, dst_lat, dst_lon, cos_dst_lat, graph, maxSpeedMs),
             0u, src_node, EDGE_NONE});

    uint64_t dst_key = UINT64_MAX;

    while (!pq.empty())
    {
        AStarState top = pq.top(); pq.pop();
        uint32_t u = top.node;
        uint32_t in_edge = top.edge_in;
        uint64_t key = stateKey(u, in_edge);

        // Lazy deletion: discard stale priority-queue entries and states that
        // were already expanded (their g cost is final).
        uint32_t slot = states.findSlot(key);
        if (slot == UINT32_MAX || top.g > states.gAt(slot))
            continue;
        if (states.expandedAt(slot))
            continue;
        states.markExpandedAt(slot);
        if (u == dst_node)
        {
            dst_key = key;
            break;
        }

        RouteEdge edge_buf[MAX_EDGES_PER_NODE_GL];
        uint32_t edge_count = 0;
        if (!graph.getEdgesForNode(u, edge_buf, edge_count))
            continue;

        uint32_t current_g = states.gAt(slot);

        // Previous node (source of the incoming edge) — for the turn penalty.
        // From the parent state key: (prev_node << 32) | prev_edge.
        uint32_t prev_node = u;
        {
            uint64_t parent_key = states.parentAt(slot);
            if (parent_key != UINT64_MAX)
                prev_node = (uint32_t)(parent_key >> 32);
        }

        float ulat, ulon;
        bool have_ulat = graph.getNodeCoords(u, ulat, ulon);
        // cos(lat) for the turn vectors — hoisted once per expanded node.
        const float cos_u_lat = have_ulat ? cosf(ulat * 3.14159265f / 180.f) : 0.f;

        // Turn penalty only matters at junctions (degree >= 3): on a degree-2
        // node the only other edge is straight ahead or a U-turn, never a sharp
        // turn, so skip the coordinate fetches and angle computation entirely.
        bool want_penalty = (in_edge != EDGE_NONE && edge_count > 2 &&
                             prev_node != u && have_ulat);
        float plat = 0.f, plon = 0.f;
        if (want_penalty && !graph.getNodeCoords(prev_node, plat, plon))
            want_penalty = false;

        for (uint32_t ei = 0; ei < edge_count; ++ei)
        {
            const RouteEdge& e = edge_buf[ei];
            // Turn restriction: (in_edge -> u) through u forbidden?
            uint32_t e_global = graph.edgeGlobalForNode(u, ei);
            if (in_edge != EDGE_NONE &&
                graph.isTurnForbidden(u, in_edge, e_global))
            {
                continue;
            }

            uint32_t add = 0;
            if (want_penalty)
            {
                float alat, alon;
                if (graph.getNodeCoords(e.dst_node, alat, alon))
                {
                    // Sharp turn (>45 deg) iff the projected dot product of
                    // (incoming, outgoing) directions is below cos(45 deg).
                    float ax = (ulon - plon) * cos_u_lat;
                    float ay = ulat - plat;
                    float bx = (alon - ulon) * cos_u_lat;
                    float by = alat - ulat;
                    float na = sqrtf(ax * ax + ay * ay);
                    float nb = sqrtf(bx * bx + by * by);
                    if (na > 1e-6f && nb > 1e-6f &&
                        (ax * bx + ay * by) / (na * nb) < TURN_MIN_ANGLE_COS)
                        add = TURN_PENALTY_TENTHS;
                }
            }

            uint32_t ng = current_g + e.cost + add;
            uint64_t nkey = stateKey(e.dst_node, e_global);
            uint32_t neighbor_g = states.getG(nkey);
            if (ng < neighbor_g)
            {
                states.upsert(nkey, ng, key);
                uint32_t h = heuristic(e.dst_node, dst_lat, dst_lon,
                                       cos_dst_lat, graph, maxSpeedMs);
                pq.push({ng + h, ng, e.dst_node, e_global});
            }

        }
    }

    if (dst_key == UINT64_MAX || states.getG(dst_key) == INF)
        return result;

    uint64_t cur = dst_key;
    while (cur != UINT64_MAX)
    {
        uint32_t n = (uint32_t)(cur >> 32);
        float nlat, nlon;
        if (!graph.getNodeCoords(n, nlat, nlon))
            break;
        wayPoint wp{};
        wp.lat = nlat;
        wp.lon = nlon;
        result.push_back(wp);
        uint64_t parent_key = states.getPrev(cur);
        cur = (parent_key != UINT64_MAX) ? parent_key : UINT64_MAX;
    }
    std::reverse(result.begin(), result.end());
    return result;
}
