#ifndef maps_h_
#define maps_h_
#include <stdint.h>
#include <vector>
#include <array>
#include <math.h>
#include <map>
#include "utils/graphics/graphics.h"

#define MAPBLOCKS_MAX 6         // max blocks in memory
#define MAPBLOCK_SIZE_BITS 12   // 4096 x 4096 coords (~meters) per block  
#define MAPFOLDER_SIZE_BITS 4   // 16 x 16 map blocks per folder
const int32_t mapblock_mask  = pow( 2, MAPBLOCK_SIZE_BITS) - 1;     // ...00000000111111111111
const int32_t mapfolder_mask = pow( 2, MAPFOLDER_SIZE_BITS) - 1;    // ...00001111


/// @brief Map square area of aprox 4096 meters side. Correspond to one single map file.
struct MapBlock {
    Point32 offset;
    BBox bbox;
    bool inView = false;
    std::vector<Polyline> polylines;
    std::vector<Polygon> polygons;
};

/// @brief MapBlocks stored in memory
struct MemBlocks {
    std::map<String, u_int16_t> blocks_map;     // block offset -> block index
    std::array<MapBlock*, MAPBLOCKS_MAX> blocks;
};

void get_map_blocks( MemBlocks& memBlocks, BBox& bbox);

#endif