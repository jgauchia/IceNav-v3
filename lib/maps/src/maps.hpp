/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "mapCanvas.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"
#include "nav_reader.hpp"
#include "PsramAllocator.hpp"

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "driver/ppa.h"
#include "esp_cache.h"
#endif

/**
 * @class Maps
 * @brief Class for handling map rendering and display
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
        int16_t posX;
        int16_t posY;
    };

    struct LabelRect
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
    };

    static const uint16_t mapTileSize = 256;
    tileBounds totalBounds;
    uint16_t wptPosX;
    uint16_t wptPosY;
    MapCanvas mapTempSprite = MapCanvas(mapCanvasParent());
    MapCanvas mapSprite = MapCanvas(mapCanvasParent());
    MapCanvas pngStagingSprite = MapCanvas(mapCanvasParent());
    uint32_t pngStagedHash = 0;
    bool pngStagingValid = false;
    float destLat = 0.0f;
    float destLon = 0.0f;
    bool hasWaypoint = false;
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
    void coords2map(float lat, float lon, const tileBounds& bound, uint16_t *pixelX, uint16_t *pixelY);
    void panMap(int8_t dx, int8_t dy);
    uint16_t darkenRGB565(const uint16_t color, const float amount = 0.4f);
    void fillPolygonGeneral(MapCanvas &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount = 1, const uint16_t* ringEnds = nullptr);

    float mapTilt;
    float focalLength;
    bool scrolling = false;
    bool inertia = false;
    bool use3DCache = false;

    bool isNavActive() const;
    void update3DCache();
    void apply3DPerspective(uint16_t heading);
    void preloadTiles(int8_t dirX, int8_t dirY);
    void preloadVectorTiles(int8_t dirX, int8_t dirY, uint8_t stepCount = 1);
    bool renderVectorViewport(float centerLat, float centerLon, uint8_t zoom, MapCanvas &map);
    void renderVectorTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map);

public:
    uint16_t tileWidth;
    uint16_t tileHeight;
    uint8_t tilesGrid;

    void* mapBuffer;
    uint16_t mapScrHeight;
    uint16_t mapScrWidth;
    volatile bool redrawMap = true;
    bool followGps = true;
    float manualHeading = 0.0f;
    bool isMapFound = false;
    MapTile oldMapTile;
    MapTile currentMapTile;
    int16_t tileX = 0;
    int16_t tileY = 0;
    int16_t lastTileX = 0;
    int16_t lastTileY;
    uint16_t lastRenderedHeading;
    ScreenCoord lastRenderedArrowPos;
    int16_t lastRenderedDisplayOffsetX;
    int16_t lastRenderedDisplayOffsetY;
    float lastRenderedManualHeading = 0.0f;
    int16_t offsetX = 0;
    int16_t offsetY = 0;
    int16_t displayOffsetX = 0;
    int16_t displayOffsetY = 0;
    int16_t pendingDx = 0;
    int16_t pendingDy = 0;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    const float friction = 0.95f;
    bool scrollUpdated = false;

    EventGroupHandle_t mapEventGroup;
    static const uint32_t MAP_EVENT_START = (1 << 0);
    static const uint32_t MAP_EVENT_DONE  = (1 << 1);
    static const uint32_t MAP_EVENT_ERROR = (1 << 2);

    Maps();
    MapTile getMapTile(float lon, float lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY);
    void initMap(uint16_t mapWidth, uint16_t mapHeight);
    void deleteMapScrSprites();
    void createMapScrSprites();
    void generateMap(uint8_t zoom);
    void displayMap();
    void setWaypoint(float wptLat, float wptLon);
    bool getHasWaypoint() const { return hasWaypoint; }
    void updateMap();
    void centerOnGps(float lat, float lon);
    void scrollMap(int16_t dx, int16_t dy);
    void commitScroll();
    void setInertia(bool active) { inertia = active; }
    void resetScrollState();

private:
    void initResources();

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
        uint8_t x1;
        uint8_t y1;
        uint8_t x2;
        uint8_t y2;
        uint8_t priority;
    };

    struct NavDataCache
    {
        uint8_t* data;
        size_t size;
        uint32_t tileHash;
        uint32_t lastAccess;
        bool isPinned;
        uint8_t pinLeft;
    };

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    static const uint8_t NAV_DATA_CACHE_SIZE = 48;
#else
    static const uint8_t NAV_DATA_CACHE_SIZE = 12;
#endif
    std::vector<NavDataCache, PsramAllocator<NavDataCache>> vectorCache;
    uint32_t cacheCounter = 0;
    uint32_t lastPrefetchHash = 0;

    static const uint16_t MAX_POLYGON_POINTS = 1024;
    static const uint32_t MAX_FEATURE_POOL_SIZE = 16384;
    static const uint16_t MAX_PLACED_LABELS = 512;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    ppa_client_handle_t ppaFillClient = nullptr;
    ppa_client_handle_t ppaBlendClient = nullptr;
#endif

    std::vector<int, PsramAllocator<int>> projBuf32X;
    std::vector<int, PsramAllocator<int>> projBuf32Y;
    std::vector<int16_t, PsramAllocator<int16_t>> decodedCoords;
    std::vector<FeatureRef, PsramAllocator<FeatureRef>> featurePool;
    std::vector<uint16_t, PsramAllocator<uint16_t>> layers[16];
    std::vector<uint16_t, PsramAllocator<uint16_t>> layersCasing[16];
    std::vector<uint16_t, PsramAllocator<uint16_t>> layersText[16];
    std::vector<uint16_t, PsramAllocator<uint16_t>> ringEndsCache;
    std::vector<LabelRect, PsramAllocator<LabelRect>> placedLabelsCache;

    void renderVectorLine(const FeatureRef& ref, MapCanvas& map, bool isCasing = false);
    void renderVectorPolygon(const FeatureRef& ref, MapCanvas& map);
    void renderVectorPoint(const FeatureRef& ref, MapCanvas& map);
    void renderVectorText(const FeatureRef& ref, MapCanvas& map, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels);
    void latLonToPixel(float lat, float lon, int16_t& px, int16_t& py);
    void drawTrack(MapCanvas& map);
    void drawWaypoint(MapCanvas& map);

public:
    void redrawTrack();
    bool isRendering() const { return pendingTilesNotEmpty; }
    bool isScrollDeferred() const { return vectorDeferred; }
    bool is3DActive() const { return use3DCache; }
    TaskHandle_t renderTaskHandle() const { return mapRenderTaskHandle; }

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

    struct VectorStep
    {
        int8_t dirX;
        int8_t dirY;
        std::vector<PendingTile> tiles;
    };

    std::vector<PendingTile> pendingTiles;
    std::vector<VectorStep> vectorSteps;
    SemaphoreHandle_t mapMutex;
    TaskHandle_t mapRenderTaskHandle;
    static void mapRenderTask(void* pvParameters);
    void renderPngTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map);
    void prefetchPngTile();
    bool tryApplyStagedPng(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map);
    bool loadPngTileIntoSprite(int32_t tlX, int32_t tlY, int gx, int gy,
                               uint32_t centerTileIdxX, uint32_t centerTileIdxY,
                               uint8_t zoom, bool& centerFound);
    void enqueueTileGrid(uint32_t centerTileIdxX, uint32_t centerTileIdxY, TileType type);
    void queueVectorStep(uint32_t centerTileIdxX, uint32_t centerTileIdxY,
                         int8_t dirX, int8_t dirY);
    uint8_t* vectorCacheLookupOrLoad(uint32_t tileX, uint32_t tileY, uint8_t zoom, size_t& outDataSize);
    void prefetchNextTile();
    void decodeVectorFeatures(const uint8_t* data, size_t dataSize, int16_t screenX, int16_t screenY, uint8_t zoom);
    static void drawThickLine(MapCanvas& map, int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1, uint8_t width, uint16_t color);

    uint8_t vectorZoom;
    bool vectorNeedsRender;
    bool vectorDeferred = false;
    bool vectorPending = false;
    bool vectorCapped = false;
    int8_t vectorDirX = 0;
    int8_t vectorDirY = 0;
    float mapTlX;
    float mapTlY;
    volatile bool pendingTilesNotEmpty = false;

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