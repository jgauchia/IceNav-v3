/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"

/**
 * @brief Draw command types used for rendering vector graphics.
 * 
 * @details These commands are synchronized with the corresponding Python script
 * 			that generates the binary tile data. Each command corresponds to a specific
 * 			geometric drawing operation.
 */
enum DrawCommand : uint8_t
{
    DRAW_POLYLINE = 2,
    DRAW_STROKE_POLYGON = 3,
    SET_COLOR = 0x80,
    SET_COLOR_INDEX = 0x81,
};

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering, navigation, and viewport management.
 *
 */
class Maps
{
    private:  
        struct MapTile 			/**< Tile Map structure */
        {
            char file[255];		/**< File path of the map tile */
            uint32_t tilex;     /**< X index of the tile */
            uint32_t tiley;     /**< Y index of the tile */
            uint8_t zoom;       /**< Zoom level of the tile */
            float lat;          /**< Latitude of the tile center */
            float lon;          /**< Longitude of the tile center */
        };
        
        struct CachedTile		/**< Cached tile structure */
        {
            TFT_eSprite* sprite;    /**< Rendered tile sprite */
            uint32_t tileHash;     /**< Hash for tile identification */
            uint32_t lastAccess;   /**< Last access timestamp for LRU */
            bool isValid;          /**< Cache entry validity */
            char filePath[255];    /**< Original file path */
        };

        struct PolygonBounds	/**< Polygon bounding box structure */
        {
            int minX, minY, maxX, maxY; /**< Bounding box coordinates */
            bool isValid;               /**< Bounds are valid */
        };
        
        struct tileBounds		/**< Map boundaries structure */
        {
            float lat_min;	   /**< Minimum latitude */
            float lat_max;     /**< Maximum latitude */
            float lon_min;     /**< Minimum longitude */
            float lon_max;     /**< Maximum longitude */
        };

        struct ScreenCoord		/**< Screen position from GPS coordinates */
        {
            uint16_t posX;		/**< X position on screen */
            uint16_t posY; 		/**< Y position on screen */
        };

        static constexpr int TILE_SIZE = 255;
        static constexpr int TILE_SIZE_PLUS_ONE = 256;
        static constexpr int MARGIN_PIXELS = 1;
        static const uint16_t tileHeight = 768;                                      /**< Tile 9x9 Height Size */
        static const uint16_t tileWidth = 768;                                       /**< Tile 9x9 Width Size */
        static const uint16_t mapTileSize = 256;                             	     /**< Map tile size */
        static const uint16_t scrollThreshold = mapTileSize / 2;                     /**< Smooth scroll threshold */

        static uint16_t currentDrawColor;                           				/**< Current drawing color state */
        static uint8_t PALETTE[256];                                                /**< Color palette for vector tiles */
        static uint32_t PALETTE_SIZE;                                               /**< Number of entries in the palette */
        
        // Tile cache system
        static std::vector<CachedTile> tileCache;                                   /**< Tile cache storage */
        static size_t maxCachedTiles;                                               /**< Maximum cached tiles based on hardware */
        static uint32_t cacheAccessCounter;                                         /**< Counter for LRU algorithm */
        
        // Background preload system
        
        // Unified memory pool system (experimental)
        struct UnifiedPoolEntry
        {
            void* ptr;
            size_t size;
            bool isInUse;
            uint32_t allocationCount;
            uint8_t type; // 0=general, 1=point, 2=command, 3=coords, 4=feature, 5=lineSegment, 6=coordArray
        };

        static std::vector<UnifiedPoolEntry> unifiedPool;                         /**< Unified memory pool */
        static SemaphoreHandle_t unifiedPoolMutex;                                /**< Mutex for unified pool */
        static size_t maxUnifiedPoolEntries;                                      /**< Maximum unified pool entries */
        static uint32_t unifiedPoolHitCount;                                      /**< Unified pool hits */
        static uint32_t unifiedPoolMissCount;                                     /**< Unified pool misses */
        
        
        // Memory monitoring and statistics
        static uint32_t totalMemoryAllocations;                                    /**< Total memory allocations */
        static uint32_t totalMemoryDeallocations;                                 /**< Total memory deallocations */
        static uint32_t peakMemoryUsage;                                           /**< Peak memory usage */
        static uint32_t currentMemoryUsage;                                        /**< Current memory usage */
        static uint32_t poolEfficiencyScore;                                       /**< Pool efficiency score (0-100) */
        static uint32_t lastStatsUpdate;                                           /**< Last statistics update timestamp */
        
        // Polygon optimization system
        static bool polygonCullingEnabled;                                         /**< Enable polygon culling */
        static bool optimizedScanlineEnabled;                                      /**< Enable optimized scanline */
        static uint32_t polygonRenderCount;                                        /**< Total polygons rendered */
        static uint32_t polygonCulledCount;                                        /**< Polygons culled (not rendered) */
        static uint32_t polygonOptimizedCount;                                     /**< Polygons using optimized algorithms */

        tileBounds totalBounds; 													/**< Map boundaries */
        uint16_t wptPosX, wptPosY;                                                  /**< Waypoint position on screen map */
        TFT_eSprite mapTempSprite = TFT_eSprite(&tft);                              /**< Full map sprite (not showed) */
        TFT_eSprite mapSprite = TFT_eSprite(&tft);                                  /**< Screen map sprite (showed) */
        float destLat, destLon;                                                     /**< Waypoint destination latitude and longitude */
        uint8_t zoomLevel;                                                          /**< Zoom level for map display */
        ScreenCoord navArrowPosition; 												/**< Navigation Arrow position on screen */

        static uint16_t lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize);
        static uint16_t lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize);
        static uint32_t lon2tilex(float f_lon, uint8_t zoom);
        static uint32_t lat2tiley(float f_lat, uint8_t zoom);
        static float tilex2lon(uint32_t tileX, uint8_t zoom);
        static float tiley2lat(uint32_t tileY, uint8_t zoom);

        tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom);
        bool isCoordInBounds(float lat, float lon, tileBounds bound);
        ScreenCoord coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize);
        void coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY);
        void showNoMap(TFT_eSprite &map);
        void panMap(int8_t dx, int8_t dy);

        bool loadPalette(const char* palettePath);											/**< Load color palette from binary file */
        uint8_t paletteToRGB332(const uint32_t idx);					 					/**< Convert palette index to RGB332 color */
        uint8_t darkenRGB332(const uint8_t color, const float amount);  					/**< Darken RGB332 color by a given fraction */
        uint16_t RGB332ToRGB565(const uint8_t color);										/**< Convert RGB332 color to RGB565 */
        uint32_t readVarint(const uint8_t* data, size_t& offset, size_t dataSize);			/**< Read a varint-encoded uint32_t from data buffer */
        int32_t readZigzag(const uint8_t* data, size_t& offset, const size_t dataSize);		/**< Read a zigzag-encoded int32_t from data buffer */
        int uint16ToPixel(const int32_t val);												/**< Convert uint16_t tile coordinate to pixel coordinate */
        bool isPointOnMargin(const int px, const int py);									/**< Check if a point is on the margin of the tile */
        bool isNear(int val, int target, int tol);											/**< Check if a value is near a target within a tolerance */
        bool shouldDrawLine(const int px1, const int py1, const int px2, const int py2);	/**< Determine if a line should be drawn based on its endpoints */
        void fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, 
                                const int numPoints, const uint16_t color,	
                                const int xOffset, const int yOffset);						/**< Fill a polygon using the scanline algorithm */
        void drawPolygonBorder(TFT_eSprite &map, const int *px, const int *py,
                                const int numPoints, const uint16_t borderColor, const uint16_t fillColor,
                                const int xOffset, const int yOffset);						/**< Draw the border of a polygon with margin handling */
        
        
        // Tile cache methods
        void initTileCache();                                                         /**< Initialize tile cache system */
        bool getCachedTile(const char* filePath, TFT_eSprite& target, int16_t xOffset, int16_t yOffset); /**< Get tile from cache */
        void addToCache(const char* filePath, TFT_eSprite& source);                 /**< Add rendered tile to cache */
        void evictLRUTile();                                                         /**< Remove least recently used tile from cache */
        void clearTileCache();                                                       /**< Clear all cached tiles */
        uint32_t calculateTileHash(const char* filePath);                           /**< Calculate hash for tile identification */
        size_t getCacheMemoryUsage();                                               /**< Get current cache memory usage in bytes */
        
        // SD optimization methods
        void prefetchAdjacentTiles(int16_t centerX, int16_t centerY, uint8_t zoom); /**< Prefetch adjacent tiles for faster loading */
    
    
    // Unified memory pool methods (experimental)
    public:
        void initUnifiedPool();                                                     /**< Initialize unified memory pool */
        static void* unifiedAlloc(size_t size, uint8_t type = 0);                          /**< Allocate from unified pool */
        static void unifiedDealloc(void* ptr);                                             /**< Deallocate from unified pool */
    
        // RAII Memory Guard for automatic memory management
        template<typename T>
        class MemoryGuard
        {
            private:
                T* ptr;
                size_t size;
                uint8_t type;
                bool fromPool;
                
            public:
                MemoryGuard(size_t numElements, uint8_t poolType = 0) 
                    : ptr(nullptr), size(numElements * sizeof(T)), type(poolType), fromPool(false)
                {
                    ptr = static_cast<T*>(Maps::unifiedAlloc(size, type));
                    if (ptr) 
                        fromPool = true;
                    else 
                    {
                        // Fallback to direct allocation
                        #ifdef BOARD_HAS_PSRAM
                            ptr = static_cast<T*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
                        #else
                            ptr = static_cast<T*>(heap_caps_malloc(size, MALLOC_CAP_8BIT));
                        #endif
                        fromPool = false;
                    }
                }
                
                ~MemoryGuard()
                {
                    if (ptr) 
                    {
                        if (fromPool)
                            Maps::unifiedDealloc(ptr);
                        else 
                            heap_caps_free(ptr);
                    }
                }
                
                T* get() const { return ptr; }
                T& operator*() const { return *ptr; }
                T* operator->() const { return ptr; }
                operator bool() const { return ptr != nullptr; }
                
                // Disable copy constructor and assignment
                MemoryGuard(const MemoryGuard&) = delete;
                MemoryGuard& operator=(const MemoryGuard&) = delete;
        };

        bool fillPolygons;                                             /**< Flag for polygon filling */
        void* mapBuffer;                                               /**< Pointer to map screen sprite */
        uint16_t mapScrHeight;                                         /**< Screen map size height */
        uint16_t mapScrWidth;                                          /**< Screen map size width */
        bool redrawMap = true;                                         /**< Flag to indicate need redraw Map */
        bool followGps = true;                                         /**< Flag to indicate if map follow GPS signal */
        bool isMapFound = false;                                       /**< Flag to indicate when map is found on SD */
        MapTile oldMapTile;                                            /**< Old Map tile coordinates and zoom */
        MapTile currentMapTile;                                        /**< Current Map tile coordinates and zoom */
        MapTile roundMapTile;                                          /**< Boundaries Map tiles */
        int8_t tileX = 0;                                              /**< Map tile x counter */
        int8_t tileY = 0;                                              /**< Map tile y counter */
        int16_t offsetX = 0;                                           /**< Accumulative X scroll map offset */
        int16_t offsetY = 0;                                           /**< Accumulative Y scroll map offset */
        bool scrollUpdated = false;                                    /**< Flag to indicate when map was scrolled and needs to update */
        int8_t lastTileX = 0;                                          /**< Last Map tile x counter */
        int8_t lastTileY = 0;                                          /**< Last Map tile y counter */


        Maps();
        MapTile getMapTile(float lon, float lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY);
        void initMap(uint16_t mapHeight, uint16_t mapWidth);
        void deleteMapScrSprites();
        void createMapScrSprites();
        void generateMap(uint8_t zoom);
        void displayMap();
        void setWaypoint(float wptLat, float wptLon);
        void updateMap();
        void centerOnGps(float lat, float lon);
        void scrollMap(int16_t dx, int16_t dy);
        void preloadTiles(int8_t dirX, int8_t dirY);
        bool renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
};

