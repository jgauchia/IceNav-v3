/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.4
 * @date 2025-11
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
 */
enum DrawCommand : uint8_t 
{
    DRAW_LINE = 1,               /**< Draw a simple line segment */
    DRAW_POLYLINE = 2,           /**< Draw a series of connected line segments */
    DRAW_STROKE_POLYGON = 3,     /**< Draw only the stroke (outline) of a single polygon */
    DRAW_STROKE_POLYGONS = 4,    /**< Draw only the strokes (outlines) of multiple polygons */
    DRAW_HORIZONTAL_LINE = 5,    /**< Draw a horizontal line */
    DRAW_VERTICAL_LINE = 6,      /**< Draw a vertical line */
    SET_COLOR = 0x80,            /**< Set current drawing color (RGB332) */
    SET_COLOR_INDEX = 0x81,      /**< Set current color using palette index */
    RECTANGLE = 0x82,            /**< Optimized rectangle command */
    STRAIGHT_LINE = 0x83,        /**< Optimized straight line command */
    HIGHWAY_SEGMENT = 0x84,      /**< Highway segment with continuity */
    GRID_PATTERN = 0x85,         /**< Urban grid pattern */
    BLOCK_PATTERN = 0x86,        /**< City block pattern */
    CIRCLE = 0x87,               /**< Optimized circle command */
    SET_LAYER = 0x88,            /**< Layer indicator command */
    RELATIVE_MOVE = 0x89,        /**< Relative coordinate movement */
    PREDICTED_LINE = 0x8A,       /**< Predictive line based on pattern */
    COMPRESSED_POLYLINE = 0x8B,  /**< Huffman-compressed polyline */
    OPTIMIZED_POLYGON = 0x8C,    /**< Optimized polygon (contour only, fill decided by viewer) */
    HOLLOW_POLYGON = 0x8D,       /**< Polygon outline only (optimized for boundaries) */
    OPTIMIZED_TRIANGLE = 0x8E,   /**< Optimized triangle (contour only, fill decided by viewer) */
    OPTIMIZED_RECTANGLE = 0x8F,  /**< Optimized rectangle (contour only, fill decided by viewer) */
    OPTIMIZED_CIRCLE = 0x90,     /**< Optimized circle (contour only, fill decided by viewer) */
    SIMPLE_RECTANGLE = 0x96,     /**< Simple rectangle (x, y, width, height) */
    SIMPLE_CIRCLE = 0x97,        /**< Simple circle (center_x, center_y, radius) */
    SIMPLE_TRIANGLE = 0x98,      /**< Simple triangle (x1, y1, x2, y2, x3, y3) */
    DASHED_LINE = 0x99,          /**< Dashed line with pattern */
    DOTTED_LINE = 0x9A,          /**< Dotted line with pattern */
};

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering.
 */
class Maps
{
    private:  
        struct MapTile {
            char file[255];
            uint32_t tilex;
            uint32_t tiley;
            uint8_t zoom;
            float lat;
            float lon;
        };
        
        struct CachedTile {
            TFT_eSprite* sprite;
            uint32_t tileHash;
            uint32_t lastAccess;
            bool isValid;
            char filePath[255];
        };
        
        struct LineSegment {
            int x0, y0, x1, y1;
            uint16_t color;
        };
        
        struct RenderBatch {
            LineSegment* segments;
            size_t count;
            size_t capacity;
            uint16_t color;
        };
        
        struct PolygonBounds {
            int minX, minY, maxX, maxY;
            bool isValid;
        };
        
        struct tileBounds {
            float lat_min;
            float lat_max;
            float lon_min;
            float lon_max;
        };

        struct ScreenCoord {
            uint16_t posX;
            uint16_t posY; 
        };

        static constexpr int TILE_SIZE = 255;
        static constexpr int TILE_SIZE_PLUS_ONE = 256;
        static constexpr int MARGIN_PIXELS = 1;
        static const uint16_t tileHeight = 768;
        static const uint16_t tileWidth = 768;
        static const uint16_t mapTileSize = 256;
        static const uint16_t scrollThreshold = mapTileSize / 2;

        static uint16_t currentDrawColor;
        static uint8_t PALETTE[256];
        static uint32_t PALETTE_SIZE;
        
        // Tile cache system
        static std::vector<CachedTile> tileCache;
        static size_t maxCachedTiles;
        static uint32_t cacheAccessCounter;
        
        // Unified memory pool system
        struct UnifiedPoolEntry {
            void* ptr;
            size_t size;
            bool isInUse;
            uint32_t allocationCount;
            uint8_t type; 
        };

        static std::vector<UnifiedPoolEntry> unifiedPool;
        static SemaphoreHandle_t unifiedPoolMutex;
        static size_t maxUnifiedPoolEntries;
        static uint32_t unifiedPoolHitCount;
        static uint32_t unifiedPoolMissCount;
        
        // Memory monitoring
        static uint32_t totalMemoryAllocations;
        static uint32_t totalMemoryDeallocations;
        static uint32_t peakMemoryUsage;
        static uint32_t currentMemoryUsage;
        static uint32_t poolEfficiencyScore;
        static uint32_t lastStatsUpdate;
        
        // Polygon optimization
        static bool polygonCullingEnabled;
        static bool optimizedScanlineEnabled;
        static uint32_t polygonRenderCount;
        static uint32_t polygonCulledCount;
        static uint32_t polygonOptimizedCount;
        
        // Batch rendering
        static RenderBatch* activeBatch;
        static size_t maxBatchSize;
        static uint32_t batchRenderCount;
        static uint32_t batchOptimizationCount;
        static uint32_t batchFlushCount;
        
        tileBounds totalBounds;
        uint16_t wptPosX, wptPosY;
        TFT_eSprite mapTempSprite = TFT_eSprite(&tft);
        TFT_eSprite mapSprite = TFT_eSprite(&tft);
        float destLat, destLon;
        uint8_t zoomLevel;
        ScreenCoord navArrowPosition;

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

        bool loadPalette(const char* palettePath);
        uint8_t paletteToRGB332(const uint32_t idx);
        uint8_t darkenRGB332(const uint8_t color, const float amount);
        uint16_t RGB332ToRGB565(const uint8_t color);
        uint32_t readVarint(const uint8_t* data, size_t& offset, size_t dataSize);
        int32_t readZigzag(const uint8_t* data, size_t& offset, const size_t dataSize);
        int uint16ToPixel(const int32_t val);
        bool isPointOnMargin(const int px, const int py);
        bool isNear(int val, int target, int tol);
        bool shouldDrawLine(const int px1, const int py1, const int px2, const int py2);
        void fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, 
                                const int numPoints, const uint16_t color,	
                                const int xOffset, const int yOffset);
        void drawPolygonBorder(TFT_eSprite &map, const int *px, const int *py, 
                                const int numPoints, const uint16_t borderColor, const uint16_t fillColor, 
                                const int xOffset, const int yOffset);
        
        void initTileCache();
        bool getCachedTile(const char* filePath, TFT_eSprite& target, int16_t xOffset, int16_t yOffset);
        void addToCache(const char* filePath, TFT_eSprite& source);
        void evictLRUTile();
        void clearTileCache();
        uint32_t calculateTileHash(const char* filePath);
        size_t getCacheMemoryUsage();
        
        void prefetchAdjacentTiles(int16_t centerX, int16_t centerY, uint8_t zoom);
    
    public:
        void initUnifiedPool();
        static void* unifiedAlloc(size_t size, uint8_t type = 0);
        static void unifiedDealloc(void* ptr);
    
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
                    if (ptr) fromPool = true;
                    else {
                        #ifdef BOARD_HAS_PSRAM
                            ptr = static_cast<T*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
                        #else
                            ptr = static_cast<T*>(heap_caps_malloc(size, MALLOC_CAP_8BIT));
                        #endif
                        fromPool = false;
                    }
                }
                ~MemoryGuard() {
                    if (ptr) {
                        if (fromPool) Maps::unifiedDealloc(ptr);
                        else heap_caps_free(ptr);
                    }
                }
                T* get() const { return ptr; }
                T& operator*() const { return *ptr; }
                T* operator->() const { return ptr; }
                operator bool() const { return ptr != nullptr; }
                MemoryGuard(const MemoryGuard&) = delete;
                MemoryGuard& operator=(const MemoryGuard&) = delete;
        };
    
        void initBatchRendering();
        void createRenderBatch(size_t capacity);
        void addToBatch(int x0, int y0, int x1, int y1, uint16_t color);
        void flushBatch(TFT_eSprite& map, int& optimizations);
        bool canBatch(uint16_t color);
        size_t getOptimalBatchSize();

        bool fillPolygons;
        void* mapBuffer;
        uint16_t mapScrHeight;
        uint16_t mapScrWidth;
        bool redrawMap = true;
        bool followGps = true;
        bool isMapFound = false;
        MapTile oldMapTile;
        MapTile currentMapTile;
        MapTile roundMapTile;
        int8_t tileX = 0;
        int8_t tileY = 0;
        int16_t offsetX = 0;
        int16_t offsetY = 0;
        bool scrollUpdated = false;
        int8_t lastTileX = 0;
        int8_t lastTileY = 0;

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