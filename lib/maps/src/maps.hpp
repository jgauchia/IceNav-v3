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
#include <algorithm>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"
#include "nav_reader.hpp"
#include "PsramAllocator.hpp"

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering, navigation, and viewport management.
 */
class Maps
{
    private:
        struct MapTile
        {
            char file[255];
            uint32_t tilex;
            uint32_t tiley;
            uint8_t zoom;
            float lat;
            float lon;
        };
        struct tileBounds
        {
            float lat_min;
            float lat_max;
            float lon_min;
            float lon_max;
        };
        struct ScreenCoord
        {
            uint16_t posX;
            uint16_t posY;
        };
        struct LabelRect
        {
            int16_t x, y, w, h;
        };
        static const uint16_t mapTileSize = 256;
        tileBounds totalBounds;
        uint16_t wptPosX, wptPosY;
        TFT_eSprite mapTempSprite = TFT_eSprite(&tft);
        TFT_eSprite mapSprite = TFT_eSprite(&tft);
        TFT_eSprite preloadSprite = TFT_eSprite(&tft);
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
        uint16_t darkenRGB565(const uint16_t color, const float amount = 0.4f);
        void fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount = 1, const uint16_t* ringEnds = nullptr);
    public:
#ifdef T4_S3
        static const uint16_t tileWidth = 1024;
        static const uint16_t tileHeight = 1024;
        static const uint8_t tilesGrid = 4;
#else
        static const uint16_t tileWidth = 768;
        static const uint16_t tileHeight = 768;
        static const uint8_t tilesGrid = 3;
#endif
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
        void resetScrollState();
        bool renderNavViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite &map);
        void renderNavTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map);
    private:
        struct FeatureRef
        {
            uint8_t* ptr;
            NavGeomType geomType;
            uint16_t payloadSize;
            uint16_t coordCount;
            int16_t tileOffsetX;
            int16_t tileOffsetY;
            uint16_t color;
            uint8_t width;
            bool casing;
            uint8_t x1, y1, x2, y2;
            uint8_t priority;
        };
        struct NavDataCache
        {
            uint8_t* data;
            size_t size;
            uint32_t tileHash;
            uint32_t lastAccess;
        };
        static const uint8_t NAV_DATA_CACHE_SIZE = 12;
        std::vector<NavDataCache, PsramAllocator<NavDataCache>> navDataCache;
        uint32_t cacheCounter = 0;
        static const uint16_t MAX_POLYGON_POINTS = 4096;
        static const uint16_t MAX_FEATURE_POOL_SIZE = 4096;
        std::vector<int, PsramAllocator<int>> projBuf32X;
        std::vector<int, PsramAllocator<int>> projBuf32Y;
        std::vector<int16_t, PsramAllocator<int16_t>> decodedCoords;
        std::vector<FeatureRef, PsramAllocator<FeatureRef>> featurePool;
        std::vector<uint16_t, PsramAllocator<uint16_t>> passIndices[4];
        std::vector<uint16_t, PsramAllocator<uint16_t>> ringEndsCache;
        std::vector<LabelRect, PsramAllocator<LabelRect>> placedLabelsCache;
        void renderNavFeature(const FeatureRef& ref, TFT_eSprite& map, uint8_t pass, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels);
        void renderNavLineString(const FeatureRef& ref, TFT_eSprite& map, bool isCasing = false);
        void renderNavPolygon(const FeatureRef& ref, TFT_eSprite& map);
        void renderNavPoint(const FeatureRef& ref, TFT_eSprite& map);
        void renderNavText(const FeatureRef& ref, TFT_eSprite& map, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels);
        void latLonToPixel(float lat, float lon, int16_t& px, int16_t& py);
        void drawTrack(TFT_eSprite &map);
    public:
        bool trackNeedsRedraw = false;
        void redrawTrack();
        bool isRendering() const { return !pendingTiles.empty(); }
    private:
        enum TileType
        {
            TILE_NAV,
            TILE_PNG
        };
        struct PendingTile
        {
            uint32_t x;
            uint32_t y;
            int16_t screenX;
            int16_t screenY;
            TileType type;
        };
        std::vector<PendingTile> pendingTiles;
        SemaphoreHandle_t mapMutex;
        TaskHandle_t mapRenderTaskHandle;
        static void mapRenderTask(void* pvParameters);
        void renderPngTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map);
        float navLastLat_;
        float navLastLon_;
        uint8_t navLastZoom_;
        bool navNeedsRender_;
        float navTlTileX_;
        float navTlTileY_;
        uint64_t viewportStartTime;
        uint64_t totalLoadTime;
        uint64_t totalProcessTime;
        uint64_t totalDrawTime;
        uint32_t totalFeaturesDrawn;
        uint16_t cacheHits;
        uint16_t cacheMisses;
    public:
        float velocityX = 0.0f;
        float velocityY = 0.0f;
        const float friction = 0.95f;
    private:
        struct Edge
        {
            int yMax;
            int xVal;
            int slope;
            int nextInBucket;
            int nextActive;
        };
        std::vector<int, PsramAllocator<int>> edgeBuckets;
        std::vector<Edge, PsramAllocator<Edge>> edgePool;
};