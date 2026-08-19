/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.3.0
 * @date 2026-06
 */

#include "maps.hpp"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <cmath>
#include <climits>
#include "tasks.hpp"
#include "mainScr.hpp"
#include "navContext.hpp"

#if defined(CONFIG_IDF_TARGET_ESP32P4)
static inline uint32_t rgb565_to_argb8888(uint16_t c)
{
    uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (c & 0x1F) * 255 / 31;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}
#endif
#include "../../images/src/bruj.h"
#include "../../images/src/compass.h"
#include "../../images/src/waypoint.h"
#include "../../images/src/navfinish.h"
#include "../../images/src/straight.h"
#include "../../images/src/slleft.h"
#include "../../images/src/slright.h"
#include "../../images/src/tleft.h"
#include "../../images/src/tright.h"
#include "../../images/src/uleft.h"
#include "../../images/src/uright.h"
#include "../../images/src/finish.h"
#include "../../images/src/outtrack.h"

#ifdef ENABLE_COMPASS
    extern Compass compass;
#endif
extern Gps gps;
extern Storage storage;
static const char* TAG = "MAPS";
static const uint16_t PREFETCH_MIN_SPEED_KMH = 5;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
static const uint8_t PREFETCH_PIN_FRAMES = 4;
static const uint8_t PREFETCH_MAX_LOAD_PER_PASS = 4;
#endif
static const float PREFETCH_MIN_DRAG_VELOCITY = 0.5f;

static bool aggressiveLod = false;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
/**
 * @brief PPA SRM completion callback (ISR context).
 *
 * @details Clears the in-flight flag pointed to by user_data so the render task
 *          knows the DMA copy has finished. Returns false (no yield needed).
 */
static bool ppaSrmDoneCb(ppa_client_handle_t, ppa_event_data_t*, void* user_data)
{
    if (user_data)
        *(volatile bool*)user_data = false;
    return false;
}
#endif

/**
 * @brief Map Class constructor
 */
Maps::Maps() : vectorZoom(0),
               vectorNeedsRender(true),
               mapTlX(-1),
               mapTlY(-1),
               lastRenderedHeading(0xFFFF),
               lastRenderedArrowPos({-32768, -32768}),
               lastRenderedDisplayOffsetX(-32768),
               lastRenderedDisplayOffsetY(-32768),
               mapTilt(60.0f),
               focalLength(300.0f){
    static_assert(Maps::MAX_FEATURE_POOL_SIZE <= 65535U,
        "featurePool index stored as uint16_t — pool size must not exceed 65535");
    // PSRAM reservations, mutexes and the render task are created in initMap(),
    // not here: on the ESP32-P4 the PSRAM and the scheduler are not ready during
    // global C++ constructors, so allocating SPIRAM in this ctor aborts at boot.
    }

/**
 * @brief Allocate PSRAM pools, sync primitives and start the render task.
 *
 * @details Split out of the constructor so it runs from setup(), when PSRAM and
 *          the FreeRTOS scheduler are available.
 */
void Maps::initResources()
{
    projBuf32X.reserve(MAX_POLYGON_POINTS);
    projBuf32Y.reserve(MAX_POLYGON_POINTS);
    decodedCoords.reserve(MAX_POLYGON_POINTS * 2);
    edgePool.reserve(MAX_POLYGON_POINTS);
    edgeBuckets.resize(tileHeight, -1);
    featurePool.reserve(MAX_FEATURE_POOL_SIZE);

    for (int i = 0; i < 16; i++)
    {
        layers[i].reserve(MAX_FEATURE_POOL_SIZE / 4);
        layersCasing[i].reserve(MAX_FEATURE_POOL_SIZE / 8);
        layersText[i].reserve(MAX_FEATURE_POOL_SIZE / 16);
    }

    ringEndsCache.reserve(MAX_POLYGON_POINTS);
    placedLabelsCache.reserve(MAX_PLACED_LABELS);
    vectorCache.reserve(NAV_DATA_CACHE_SIZE);
    mapMutex = xSemaphoreCreateRecursiveMutex();
    mapEventGroup = xEventGroupCreate();
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if (ppaFillClient == nullptr)
    {
        ppa_client_config_t cfg = {};
        cfg.oper_type = PPA_OPERATION_FILL;
        cfg.max_pending_trans_num = 1;
        ppa_register_client(&cfg, &ppaFillClient);
    }
    if (ppaBlendClient == nullptr)
    {
        ppa_client_config_t cfg = {};
        cfg.oper_type = PPA_OPERATION_BLEND;
        cfg.max_pending_trans_num = 1;
        ppa_register_client(&cfg, &ppaBlendClient);
    }
    if (ppaSrmClient == nullptr)
    {
        ppa_client_config_t cfg = {};
        cfg.oper_type = PPA_OPERATION_SRM;
        cfg.max_pending_trans_num = 1;
        ppa_register_client(&cfg, &ppaSrmClient);
    }
    if (ppaSrmClient != nullptr)
    {
        ppa_event_callbacks_t cbs = {};
        cbs.on_trans_done = ppaSrmDoneCb;
        ppa_client_register_event_callbacks(ppaSrmClient, &cbs);
    }
#endif
    xTaskCreatePinnedToCore(mapRenderTask, "MapRenderTask", 4096, this, 2, &mapRenderTaskHandle, 0);
}

/**
 * @brief Computes the Mercator northing (merc_n) from a latitude in degrees.
 *
 * @details Shared helper used by lat2posy() and lat2tiley() to avoid duplicating
 *          the same trigonometric projection calculation.
 *
 * @param f_lat Latitude in degrees.
 * @return Mercator northing value (natural log of the secant-tangent term).
 */
static float calcMercatorN(float f_lat)
{
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    return logf(tanf(lat_rad) + 1.0f / cosf(lat_rad));
}

/**
 * @brief Get pixel X position from longitude
 *
 * @param f_lon Longitude coordinate.
 * @param zoom Zoom level.
 * @param tileSize Size of the map tile in pixels.
 * @return Pixel X position within the tile.
 */
uint16_t Maps::lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize)
{
    uint32_t scale = 1 << zoom;
    return static_cast<uint16_t>(((f_lon + 180.0f) / 360.0f * scale * tileSize)) % tileSize;
}

/**
 * @brief Get pixel Y position from latitude
 *
 * @param f_lat Latitude coordinate.
 * @param zoom Zoom level.
 * @param tileSize Size of the map tile in pixels.
 * @return Pixel Y position within the tile.
 */
uint16_t Maps::lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize)
{
    float merc_n = calcMercatorN(f_lat);
    uint32_t scale = 1 << zoom;
    float total_scale = scale * tileSize;
    return static_cast<uint16_t>(((1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * total_scale)) % tileSize;
}

/**
 * @brief Get TileX for OSM files
 *
 * @param f_lon Longitude coordinate.
 * @param zoom Zoom level.
 * @return X tile index.
 */
uint32_t Maps::lon2tilex(float f_lon, uint8_t zoom)
{
    uint32_t scale = 1 << zoom;
    float rawTile = (f_lon + 180.0f) / 360.0f * scale;
    rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}

/**
 * @brief Get TileY for OSM files
 *
 * @param f_lat Latitude coordinate.
 * @param zoom Zoom level.
 * @return Y tile index.
 */
uint32_t Maps::lat2tiley(float f_lat, uint8_t zoom)
{
    float merc_n = calcMercatorN(f_lat);
    uint32_t scale = 1 << zoom;
    float rawTile = (1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * scale;
    rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}

/**
 * @brief Get Longitude from tile X
 *
 * @param tileX Tile X index.
 * @param zoom Zoom level.
 * @return Longitude coordinate.
 */
float Maps::tilex2lon(uint32_t tileX, uint8_t zoom)
{
    uint32_t scale = 1 << zoom;
    return static_cast<float>(tileX) * 360.0f / scale - 180.0f;
}

/**
 * @brief Get Latitude from tile Y
 *
 * @param tileY Tile Y index.
 * @param zoom Zoom level.
 * @return Latitude coordinate.
 */
float Maps::tiley2lat(uint32_t tileY, uint8_t zoom)
{
    float scale = static_cast<float>(1 << zoom);
    float n = static_cast<float>(M_PI) * (1.0f - 2.0f * static_cast<float>(tileY) / scale);
    return 180.0f / static_cast<float>(M_PI) * atanf(sinhf(n));
}

/**
 * @brief Get map tile structure from GPS Coordinates
 * 
 * @param lon Longitude
 * @param lat Latitude
 * @param zoomLevel Zoom level
 * @param offsetX X Screen tile offset
 * @param offsetY Y Screen tile offset
 * @return Maps::MapTile structure
 */
Maps::MapTile Maps::getMapTile(float lon, float lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY)
{
    MapTile data;
    data.tilex = Maps::lon2tilex(lon, zoomLevel) + offsetX;
    data.tiley = Maps::lat2tiley(lat, zoomLevel) + offsetY;
    data.zoom = zoomLevel;
    data.lat = lat; 
    data.lon = lon;
    snprintf(data.file, sizeof(data.file), mapRenderFolder, zoomLevel, data.tilex, data.tiley);
    return data;
}

/**
 * @brief Get geographic boundaries of a tile
 * 
 * @param tileX X Tile
 * @param tileY Y Tile
 * @param zoom Zoom level
 * @return Maps::tileBounds structure
 */
Maps::tileBounds Maps::getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom)
{
    tileBounds bounds;
    bounds.lon_min = Maps::tilex2lon(tileX, zoom);
    bounds.lat_min = Maps::tiley2lat(tileY + 1, zoom);
    bounds.lon_max = Maps::tilex2lon(tileX + 1, zoom);
    bounds.lat_max = Maps::tiley2lat(tileY, zoom);
    return bounds;
}

/**
 * @brief Check if coordinates are in map bounds
 * 
 * @param lat Latitude
 * @param lon Longitude
 * @param bound Tile boundaries
 * @return true if coordinates are in map bounds otherwise false
 */
bool Maps::isCoordInBounds(float lat, float lon, tileBounds bound)
{
    if (lat < bound.lat_min)
        return false;
    if (lat > bound.lat_max)
        return false;
    if (lon < bound.lon_min)
        return false;
    if (lon > bound.lon_max)
        return false;
    return true;
}

/**
 * @brief Convert GPS Coordinates to screen position
 * 
 * @param lon Longitude
 * @param lat Latitude
 * @param zoomLevel Zoom level
 * @param tileSize Tile size
 * @return Maps::ScreenCoord x,y screen position
 */
Maps::ScreenCoord Maps::coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize)
{
    ScreenCoord data;
    data.posX = Maps::lon2posx(lon, zoomLevel, tileSize);
    data.posY = Maps::lat2posy(lat, zoomLevel, tileSize);
    return data;
}

/**
 * @brief Convert coordinates to map pixels
 * 
 * @param lat Latitude
 * @param lon Longitude
 * @param bound Tile boundaries
 * @param pixelX X pixel
 * @param pixelY Y pixel
 */
void Maps::coords2map(float lat, float lon, const tileBounds& bound, uint16_t *pixelX, uint16_t *pixelY)
{
    float lon_ratio = (lon - bound.lon_min) / (bound.lon_max - bound.lon_min);
    float lat_ratio = (bound.lat_max - lat) / (bound.lat_max - bound.lat_min);
    *pixelX = (uint16_t)(lon_ratio * Maps::tileWidth);
    *pixelY = (uint16_t)(lat_ratio * Maps::tileHeight);
}

/**
 * @brief Initialize map sprites and variables
 * 
 * @param mapWidth Map width
 * @param mapHeight Map height
 */
void Maps::initMap(uint16_t mapWidth, uint16_t mapHeight)
{
    // Grid must cover the largest screen dimension with at least half a tile
    // of scroll margin on each side; grows one tile at a time from the 3x3
    // baseline instead of forcing extra tiles every board pays for.
    const uint16_t maxScreenDim = std::max(mapHeight, mapWidth);
    uint8_t neededGrid = 3;
    while ((neededGrid * mapTileSize - maxScreenDim) / 2 < mapTileSize / 2)
        neededGrid++;
    Maps::tilesGrid = neededGrid;
    Maps::tileWidth = neededGrid * mapTileSize;
    Maps::tileHeight = neededGrid * mapTileSize;

    initResources();
    Maps::mapScrHeight = mapHeight;
    Maps::mapScrWidth = mapWidth;
    // focalLength was tuned for ICENAV_BOARD's viewport (320x480 panel, minus
    // the 27px status bar). Scaling it by height keeps the ground X/Y aspect
    // ratio consistent on screens with a different width/height ratio (4.3").
    constexpr float referenceHeight = 480.0f - 27.0f;
    Maps::focalLength = 300.0f * (static_cast<float>(mapHeight) / referenceHeight);
    Maps::mapTempSprite.createSprite(Maps::tileWidth, Maps::tileHeight);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    {
        // Two aligned PSRAM buffers so the incremental scroll can be done as an
        // out-of-place PPA SRM copy followed by a buffer swap (see scrollVectorSprite).
        const size_t bufSize = Maps::tileWidth * Maps::tileHeight * 2;
        uint8_t* bufA = static_cast<uint8_t*>(heap_caps_aligned_alloc(128, bufSize, MALLOC_CAP_SPIRAM));
        if (bufA)
        {
            Maps::mapTempSprite.setBuffer(bufA, Maps::tileWidth, Maps::tileHeight);
            if (Maps::mapTempBufs[0] && Maps::mapTempBufs[0] != bufA)
                heap_caps_free(Maps::mapTempBufs[0]);
            Maps::mapTempBufs[0] = bufA;
            uint8_t* bufB = static_cast<uint8_t*>(heap_caps_aligned_alloc(128, bufSize, MALLOC_CAP_SPIRAM));
            if (Maps::mapTempBufs[1] && Maps::mapTempBufs[1] != bufB)
                heap_caps_free(Maps::mapTempBufs[1]);
            Maps::mapTempBufs[1] = bufB;
        }
    }
#endif
    Maps::pngStagingSprite.createSprite(256, 256);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    {
        uint8_t* buf = static_cast<uint8_t*>(Maps::pngStagingSprite.getBuffer());
        if (buf && ((uint32_t)buf & 0x7F) != 0)
        {
            size_t bufSize = 256 * 256 * 2;
            uint8_t* alignedBuf = static_cast<uint8_t*>(heap_caps_aligned_alloc(128, bufSize, MALLOC_CAP_SPIRAM));
            if (alignedBuf)
                Maps::pngStagingSprite.setBuffer(alignedBuf, 256, 256);
        }
    }
#endif
    // LGFX scroll() fills the vacated band with the sprite base color (map background 0xF7BE).
    Maps::mapTempSprite.setBaseColor(0xF7BE);
    Maps::mapTempSprite.loadFont("/spiffs/font/font.vlw");
    Maps::mapSprite.createSprite(mapWidth, mapHeight);
    Maps::mapBuffer = Maps::mapSprite.getBuffer();
    Maps::oldMapTile = {};
    Maps::currentMapTile = {};
    Maps::navArrowPosition = {0, 0};
    Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};
}

/**
 * @brief Delete map sprites
 */
void Maps::deleteMapScrSprites()
{
    NavReader::closePack();
}

/**
 * @brief Reassign map buffer pointer
 */
void Maps::createMapScrSprites()
{
    Maps::mapBuffer = Maps::mapSprite.getBuffer();
}

/**
 * @brief Draw current track on map
 *
 * @param map Target sprite.
 */
void Maps::drawTrack(MapCanvas& map)
{
    for (size_t i = 1; i < navCtx.trackData.size(); ++i)
    {
        const auto &p1 = navCtx.trackData[i - 1];
        const auto &p2 = navCtx.trackData[i];
        int16_t x1;
        int16_t y1;
        int16_t x2;
        int16_t y2;
        latLonToPixel(p1.lat, p1.lon, x1, y1);
        latLonToPixel(p2.lat, p2.lon, x2, y2);
        if ((x1 >= 0 && x1 < tileWidth && y1 >= 0 && y1 < tileHeight) || (x2 >= 0 && x2 < tileWidth && y2 >= 0 && y2 < tileHeight))
            map.drawWideLine(x1, y1, x2, y2, 3, 0x6298);
    }
}

/**
 * @brief Draw the waypoint marker onto the grid sprite.
 *
 * @details Stamps the marker in grid space (same coordinate frame as drawTrack) so it travels
 *          with the grid during scroll and rotation, instead of being re-stamped every frame in
 *          displayMap(). Drawn only when a waypoint is set and its grid position is in range.
 *
 * @param map Target sprite.
 */
void Maps::drawWaypoint(MapCanvas& map)
{
    if (!hasWaypoint)
        return;
    if (wptPosX >= tileWidth || wptPosY >= tileHeight)
        return;
    map.pushImage(wptPosX - 8, wptPosY - 8, 16, 16, (uint16_t *)waypoint, TFT_BLACK);
}

/**
 * @brief Request track redraw
 */
void Maps::redrawTrack()
{
    vectorNeedsRender = true;
}

/**
 * @brief Loads a single PNG tile into the map sprite and updates totalBounds.
 *
 * @param tlX Top-left tile X index of the grid.
 * @param tlY Top-left tile Y index of the grid.
 * @param gx Grid column offset (0-based).
 * @param gy Grid row offset (0-based).
 * @param centerTileIdxX Expected center tile X index.
 * @param centerTileIdxY Expected center tile Y index.
 * @param zoom Current zoom level.
 * @param centerFound Set to true if this tile is the center tile.
 * @return True if the PNG was loaded successfully.
 */
bool Maps::loadPngTileIntoSprite(int32_t tlX, int32_t tlY, int gx, int gy,
                                  uint32_t centerTileIdxX, uint32_t centerTileIdxY,
                                  uint8_t zoom, bool& centerFound)
{
    uint32_t tx = (uint32_t)(tlX + gx);
    uint32_t ty = (uint32_t)(tlY + gy);
    int16_t sx = (int16_t)(gx * mapTileSize);
    int16_t sy = (int16_t)(gy * mapTileSize);
    if (tryApplyStagedPng(tx, ty, zoom, sx, sy, mapTempSprite))
    {
        if (tx == centerTileIdxX && ty == centerTileIdxY)
            centerFound = true;
        const tileBounds currentBounds = getTileBounds(tx, ty, zoom);
        if (currentBounds.lat_min < totalBounds.lat_min)
            totalBounds.lat_min = currentBounds.lat_min;
        if (currentBounds.lat_max > totalBounds.lat_max)
            totalBounds.lat_max = currentBounds.lat_max;
        if (currentBounds.lon_min < totalBounds.lon_min)
            totalBounds.lon_min = currentBounds.lon_min;
        if (currentBounds.lon_max > totalBounds.lon_max)
            totalBounds.lon_max = currentBounds.lon_max;
        return true;
    }
    char tilePath[128];
    snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoom, tx, ty);
    if (mapTempSprite.drawPngFile(tilePath, sx, sy))
    {
        if (tx == centerTileIdxX && ty == centerTileIdxY)
            centerFound = true;
        const tileBounds currentBounds = getTileBounds(tx, ty, zoom);
        if (currentBounds.lat_min < totalBounds.lat_min)
            totalBounds.lat_min = currentBounds.lat_min;
        if (currentBounds.lat_max > totalBounds.lat_max)
            totalBounds.lat_max = currentBounds.lat_max;
        if (currentBounds.lon_min < totalBounds.lon_min)
            totalBounds.lon_min = currentBounds.lon_min;
        if (currentBounds.lon_max > totalBounds.lon_max)
            totalBounds.lon_max = currentBounds.lon_max;
        return true;
    }
    mapTempSprite.fillRect(sx, sy, mapTileSize, mapTileSize, TFT_BLACK);
    mapTempSprite.drawPngFile(noMapFile, sx + mapTileSize / 2 - 50, sy + mapTileSize / 2 - 50);
    return false;
}

/**
 * @brief Generate the map grid
 *
 * @param zoom Zoom level
 */
void Maps::generateMap(uint8_t zoom)
{
    if (zoom != Maps::zoomLevel)
    {
        Maps::zoomLevel = zoom;
        Maps::currentMapTile.zoom = zoom;
        Maps::currentMapTile.tilex = Maps::lon2tilex(Maps::currentMapTile.lon, zoom);
        Maps::currentMapTile.tiley = Maps::lat2tiley(Maps::currentMapTile.lat, zoom);
        resetScrollState();
    }

    const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
    const float baseLat = Maps::followGps ? gpsSnap.latitude : Maps::currentMapTile.lat;
    const float baseLon = Maps::followGps ? gpsSnap.longitude : Maps::currentMapTile.lon;

    if (mapSet.vectorMap)
    {
        const uint32_t centerTileIdxX = lon2tilex(baseLon, zoom);
        const uint32_t centerTileIdxY = lat2tiley(baseLat, zoom);
        const int8_t gridOffset = tilesGrid / 2;
        const int32_t currentTlX = (int32_t)centerTileIdxX - gridOffset;
        const int32_t currentTlY = (int32_t)centerTileIdxY - gridOffset;
        bool zoomChanged = (zoom != vectorZoom);
        bool tileChanged = (currentTlX != (int32_t)mapTlX || currentTlY != (int32_t)mapTlY);

        if (zoomChanged)
            vectorNeedsRender = true;

        if (vectorDeferred && !zoomChanged && !vectorNeedsRender)
            return;
        if (vectorPending)
            return;

        if (!zoomChanged && !tileChanged && !vectorNeedsRender &&
            pendingTiles.empty() && vectorSteps.empty())
            return;

        if (pendingTiles.size() > (tilesGrid * tilesGrid))
            return;

        Maps::isMapFound = renderVectorViewport(baseLat, baseLon, zoom, Maps::mapTempSprite);
        vectorZoom = zoom;
        vectorNeedsRender = false;
        latLonToPixel(destLat, destLon, (int16_t&)wptPosX, (int16_t&)wptPosY);
        Maps::redrawMap = true;
        return;
    }

    const uint32_t centerTileIdxX = lon2tilex(baseLon, zoom);
    const uint32_t centerTileIdxY = lat2tiley(baseLat, zoom);

    if (centerTileIdxX != Maps::oldMapTile.tilex || centerTileIdxY != Maps::oldMapTile.tiley || zoom != Maps::oldMapTile.zoom)
    {
        Maps::oldMapTile.tilex = centerTileIdxX;
        Maps::oldMapTile.tiley = centerTileIdxY;
        Maps::oldMapTile.zoom = zoom;
        const int8_t gridOffset = tilesGrid / 2;
        const int32_t tlX = (int32_t)centerTileIdxX - gridOffset;
        const int32_t tlY = (int32_t)centerTileIdxY - gridOffset;
        mapTlX = (float)tlX;
        mapTlY = (float)tlY;
        vectorZoom = zoom;
        Maps::isMapFound = true;
        Maps::redrawMap = true;
        if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            pendingTiles.clear();
            vectorSteps.clear();
            pendingTilesNotEmpty = false;
            enqueueTileGrid(centerTileIdxX, centerTileIdxY, TILE_PNG);
            xSemaphoreGiveRecursive(mapMutex);
        }
    }
}

/**
 * @brief Background task for map rendering.
 *
 * @details Renders full PNG/vector grids and consumes queued vector border steps without rebuilding
 *          the complete grid for each coalesced axial crossing.
 *
 * @param pvParameters Pointer to the Maps instance passed to the FreeRTOS task.
 */
void Maps::mapRenderTask(void* pvParameters)
{
    Maps* instance = static_cast<Maps*>(pvParameters);
    uint8_t lastZoom = 0;

    while (1)
    {
        if (!instance->pendingTiles.empty() || !instance->vectorSteps.empty())
        {
            if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
            {
                if (instance->mapTempSprite.getBuffer() == nullptr)
                {
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
                if (instance->pendingTiles.empty() && !instance->vectorSteps.empty())
                {
                    std::vector<PendingTile> nextTiles;
                    const int8_t nextDirX = instance->vectorSteps.front().dirX;
                    const int8_t nextDirY = instance->vectorSteps.front().dirY;
                    nextTiles.swap(instance->vectorSteps.front().tiles);
                    instance->vectorSteps.erase(instance->vectorSteps.begin());
                    instance->pendingTiles.swap(nextTiles);
                    instance->vectorDirX = nextDirX;
                    instance->vectorDirY = nextDirY;
                    instance->vectorPending = true;
                    instance->pendingTilesNotEmpty = true;
                }
                bool zoomChanged = (instance->zoomLevel != lastZoom);
                bool fullReset = zoomChanged || (instance->pendingTiles.size() >= (size_t)(instance->tilesGrid * instance->tilesGrid));
                bool vectorRender = mapSet.vectorMap && instance->vectorPending && !fullReset;
                lastZoom = instance->zoomLevel;

                if (fullReset)
                {
                    instance->vectorPending = false;
                    instance->vectorCapped = false;
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_DONE | MAP_EVENT_ERROR);
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_START);

                    if (zoomChanged)
                    {
                        for (auto& entry : instance->vectorCache)
                            heap_caps_free(entry.data);

                        instance->vectorCache.clear();
                    }

                    instance->featurePool.clear();
                    instance->decodedCoords.clear();
                    for (int i = 0; i < 16; i++)
                    {
                        instance->layers[i].clear();
                        instance->layersCasing[i].clear();
                        instance->layersText[i].clear();
                    }

                    if (mapSet.vectorMap)
                        NavReader::openPack(instance->zoomLevel);
                    else if (instance->mapTempSprite.getBuffer())
                    {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
                        uint8_t* buf = static_cast<uint8_t*>(instance->mapTempSprite.getBuffer());
                        if (((uint32_t)buf & 0x7F) == 0)
                        {
                            ppa_fill_oper_config_t cfg = {};
                            cfg.fill_argb_color.val = rgb565_to_argb8888(TFT_WHITE);
                            cfg.out.buffer = buf;
                            cfg.out.buffer_size = instance->tileWidth * instance->tileHeight * 2;
                            cfg.out.pic_w = instance->tileWidth;
                            cfg.out.pic_h = instance->tileHeight;
                            cfg.out.block_offset_x = 0;
                            cfg.out.block_offset_y = 0;
                            cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;
                            cfg.fill_block_w = instance->tileWidth;
                            cfg.fill_block_h = instance->tileHeight;
                            cfg.mode = PPA_TRANS_MODE_BLOCKING;
                            ppa_do_fill(instance->ppaFillClient, &cfg);
                            esp_cache_msync(buf, instance->tileWidth * instance->tileHeight * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
                        }
                        else
                        {
                            instance->mapTempSprite.fillSprite(TFT_WHITE);
                        }
#else
                        instance->mapTempSprite.fillSprite(TFT_WHITE);
#endif
                    }
                }

                if (vectorRender)
                {
                    const int16_t shiftX = -instance->vectorDirX * mapTileSize;
                    const int16_t shiftY = -instance->vectorDirY * mapTileSize;
                    instance->scrollVectorSprite(shiftX, shiftY);
                    for (auto& label : instance->placedLabelsCache)
                    {
                        label.x += shiftX;
                        label.y += shiftY;
                    }
                    instance->placedLabelsCache.erase(std::remove_if(instance->placedLabelsCache.begin(), instance->placedLabelsCache.end(),
                                                                      [instance](const LabelRect& label)
                                                                      {
                                                                          return label.x + label.w < 0 || label.y + label.h < 0 ||
                                                                                 label.x >= (int16_t)instance->tileWidth ||
                                                                                 label.y >= (int16_t)instance->tileHeight;
                                                                      }),
                                                       instance->placedLabelsCache.end());
                    instance->featurePool.clear();
                    instance->decodedCoords.clear();
                    for (int i = 0; i < 16; i++)
                    {
                        instance->layers[i].clear();
                        instance->layersCasing[i].clear();
                        instance->layersText[i].clear();
                    }
                }

                // Yields mutex briefly so other tasks can run between tile renders.
                // Returns true if rendering should abort (mutex lost or new viewport pending).
                // During incremental vector render the mutex is only released between border tiles
                // when the renderer is falling behind the finger (queued steps or pending scroll
                // deltas); a slow single crossing stays atomic so the incoming edge is not left
                // half-painted and no snapback occurs. The sequence aborts only if a fallback,
                // commit or full-grid reset invalidated the current band.
                bool stepBehindFinger = false;
                auto yieldTile = [&]() -> bool
                {
                    if (vectorRender)
                    {
                        const int16_t halfTile = mapTileSize / 2;
                        const bool behindFinger = !instance->vectorSteps.empty() ||
                                                  abs(instance->pendingDx) >= halfTile ||
                                                  abs(instance->pendingDy) >= halfTile;
                        if (!behindFinger)
                            return false;
                        stepBehindFinger = true;
                    }
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(1);
                    bool shouldAbort = xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE;
                    if (!shouldAbort)
                    {
                        if (vectorRender)
                        {
                            if (!instance->vectorPending)
                                shouldAbort = true;
                            else if (instance->pendingTiles.size() >= (size_t)(instance->tilesGrid * instance->tilesGrid))
                                shouldAbort = true;
                        }
                        else if (instance->pendingTiles.size() >= (size_t)(instance->tilesGrid * instance->tilesGrid))
                            shouldAbort = true;
                    }
                    return shouldAbort;
                };

                // Yields mutex briefly between feature render passes (endWrite/startWrite around the pause).
                // Returns true if rendering should abort.
                auto yieldFeature = [&]() -> bool
                {
                    if (vectorRender)
                        return false;
                    instance->mapTempSprite.endWrite();
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(pdMS_TO_TICKS(2));
                    bool shouldAbort = xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE;
                    if (!shouldAbort && !instance->pendingTiles.empty())
                        shouldAbort = true;
                    if (!shouldAbort)
                        instance->mapTempSprite.startWrite();
                    return shouldAbort;
                };

                bool aborted = false;
                while (!instance->pendingTiles.empty())
                {
                    PendingTile t = instance->pendingTiles.back();
                    instance->pendingTiles.pop_back();
                    if (instance->pendingTiles.empty() && instance->vectorSteps.empty())
                        instance->pendingTilesNotEmpty = false;
                    if (t.type == TILE_NAV)
                    {
                        instance->renderVectorTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                        if (yieldTile()) { aborted = true; break; }
                    }
                    else if (t.type == TILE_PNG)
                    {
                        instance->renderPngTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                        if (yieldTile()) { aborted = true; break; }
                    }
                }

                if (aborted)
                {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
                    // The active buffer was already swapped; never let displayMap
                    // read it while the DMA copy is still writing it.
                    while (instance->srmInFlight)
                        vTaskDelay(1);
#endif
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    continue;
                }

#if defined(CONFIG_IDF_TARGET_ESP32P4)
                // The SRM copy runs during the band decode above; wait for it to
                // finish before painting over the copied region (track/waypoint
                // draw into it too). Normally already done (decode > copy time).
                while (instance->srmInFlight)
                    vTaskDelay(1);
#endif

                if (!mapSet.vectorMap)
                {
                    const int32_t tlX = (int32_t)instance->mapTlX;
                    const int32_t tlY = (int32_t)instance->mapTlY;
                    instance->totalBounds = instance->getTileBounds((uint32_t)tlX, (uint32_t)tlY, instance->zoomLevel);
                    const tileBounds brBounds = instance->getTileBounds((uint32_t)(tlX + instance->tilesGrid - 1),
                                                                        (uint32_t)(tlY + instance->tilesGrid - 1), instance->zoomLevel);
                    if (brBounds.lat_min < instance->totalBounds.lat_min)
                        instance->totalBounds.lat_min = brBounds.lat_min;
                    if (brBounds.lat_max > instance->totalBounds.lat_max)
                        instance->totalBounds.lat_max = brBounds.lat_max;
                    if (brBounds.lon_min < instance->totalBounds.lon_min)
                        instance->totalBounds.lon_min = brBounds.lon_min;
                    if (brBounds.lon_max > instance->totalBounds.lon_max)
                        instance->totalBounds.lon_max = brBounds.lon_max;

                    if (instance->isMapFound && instance->isCoordInBounds(instance->destLat, instance->destLon, instance->totalBounds))
                        instance->coords2map(instance->destLat, instance->destLon, instance->totalBounds, &instance->wptPosX, &instance->wptPosY);
                    else
                    {
                        instance->wptPosX = -1;
                        instance->wptPosY = -1;
                    }

                    instance->displayOffsetX = instance->offsetX;
                    instance->displayOffsetY = instance->offsetY;
                    instance->lastTileX = instance->tileX;
                    instance->lastTileY = instance->tileY;

                    instance->drawTrack(instance->mapTempSprite);
                    instance->drawWaypoint(instance->mapTempSprite);
                    instance->redrawMap = true;
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    triggerMapRedraw();
                    continue;
                }

                if (!vectorRender && instance->mapTempSprite.getBuffer())
                {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
                    uint8_t* buf = static_cast<uint8_t*>(instance->mapTempSprite.getBuffer());
                    if (((uint32_t)buf & 0x7F) == 0)
                    {
                        ppa_fill_oper_config_t cfg = {};
                        cfg.fill_argb_color.val = rgb565_to_argb8888(0xF7BE);
                        cfg.out.buffer = buf;
                        cfg.out.buffer_size = instance->tileWidth * instance->tileHeight * 2;
                        cfg.out.pic_w = instance->tileWidth;
                        cfg.out.pic_h = instance->tileHeight;
                        cfg.out.block_offset_x = 0;
                        cfg.out.block_offset_y = 0;
                        cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;
                        cfg.fill_block_w = instance->tileWidth;
                        cfg.fill_block_h = instance->tileHeight;
                        cfg.mode = PPA_TRANS_MODE_BLOCKING;
                        ppa_do_fill(instance->ppaFillClient, &cfg);
                        esp_cache_msync(buf, instance->tileWidth * instance->tileHeight * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
                    }
                    else
                    {
                        instance->mapTempSprite.fillSprite(0xF7BE);
                    }
#else
                    instance->mapTempSprite.fillSprite(0xF7BE);
#endif
                }

                instance->update3DCache();
                if (!vectorRender)
                    instance->placedLabelsCache.clear();
                aggressiveLod = vectorRender && stepBehindFinger;
                if (aggressiveLod)
                    instance->vectorCapped = true;
                instance->mapTempSprite.startWrite();
                uint32_t lastYield = millis_idf();
                uint32_t loopCounter = 0;

                for (int i = 0; i < 16 && !aborted; i++)
                {
                    // While falling behind the finger, skip low-priority fill layers
                    // (landuse/forest/parks) but keep the ground layer 0 and roads+.
                    if (aggressiveLod && i > 0 && i < 7)
                        continue;
                    const auto& layer = instance->layers[i];
                    if (layer.empty())
                        continue;

                    // Pass 1: Polygons, Points, and LineString Outlines (Casing)
                    for (uint16_t idx : layer)
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis_idf();
                            if (now - lastYield > 40)
                            {
                                if (yieldFeature()) { aborted = true; break; }
                                lastYield = millis_idf();
                            }
                        }

                        const auto& feat = instance->featurePool[idx];
                        if (feat.geomType == NavGeomType::Polygon)
                        {
                            instance->renderVectorPolygon(feat, instance->mapTempSprite);
                        }
                        else if (feat.geomType == NavGeomType::Point)
                        {
                            instance->renderVectorPoint(feat, instance->mapTempSprite);
                        }
                        else if (feat.geomType == NavGeomType::LineString)
                        {
                            instance->renderVectorLine(feat, instance->mapTempSprite, feat.casing);
                        }
                    }

                    if (aborted)
                        break;

                    // Pass 2: LineString bodies (from pre-separated casing list)
                    for (uint16_t idx : instance->layersCasing[i])
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis_idf();
                            if (now - lastYield > 40)
                            {
                                if (yieldFeature()) { aborted = true; break; }
                                lastYield = millis_idf();
                            }
                        }
                        instance->renderVectorLine(instance->featurePool[idx], instance->mapTempSprite, false);
                    }

                    if (aborted)
                        break;

                }

                if (!aggressiveLod)
                {
                    for (int i = 0; i < 16 && !aborted; i++)
                    {
                        for (uint16_t idx : instance->layersText[i])
                        {
                            if ((++loopCounter & 127) == 0)
                            {
                                uint32_t now = millis_idf();
                                if (now - lastYield > 40)
                                {
                                    if (yieldFeature()) { aborted = true; break; }
                                    lastYield = millis_idf();
                                }
                            }
                            instance->renderVectorText(instance->featurePool[idx], instance->mapTempSprite, instance->placedLabelsCache);
                        }
                    }
                }

                if (aborted)
                {
                    aggressiveLod = false;
                    if (xSemaphoreGetMutexHolder(instance->mapMutex) == xTaskGetCurrentTaskHandle())
                        xSemaphoreGiveRecursive(instance->mapMutex);

                    continue;
                }

                instance->mapTempSprite.endWrite();
                aggressiveLod = false;
                const bool sequencePending = vectorRender && !instance->vectorSteps.empty();
                if (vectorRender)
                {
                    instance->vectorPending = sequencePending;
                    if (!sequencePending)
                    {
                        instance->vectorDirX = 0;
                        instance->vectorDirY = 0;
                    }
                }
                // After an aggressive sequence, repaint the full viewport once the finger
                // stops so skipped low layers and text are restored at full detail.
                if (vectorRender && instance->vectorCapped && !sequencePending)
                {
                    instance->vectorCapped = false;
                    instance->enqueueTileGrid(instance->tileX, instance->tileY, TILE_NAV);
                }
                for (auto& entry : instance->vectorCache)
                {
                    if (entry.pinLeft > 0)
                        entry.pinLeft--;
                    else
                        entry.isPinned = false;
                }

                instance->displayOffsetX = instance->offsetX;
                instance->displayOffsetY = instance->offsetY;
                instance->lastTileX = instance->tileX;
                instance->lastTileY = instance->tileY;

                instance->drawTrack(instance->mapTempSprite);
                instance->drawWaypoint(instance->mapTempSprite);
                instance->redrawMap = true;

                if (sequencePending)
                {
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_DONE);
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_START);
                }
                else
                {
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                }
                xSemaphoreGiveRecursive(instance->mapMutex);
                triggerMapRedraw();
            }
        }
        else if (mapSet.vectorMap)
            instance->prefetchNextTile();
        else
            instance->prefetchPngTile();
        vTaskDelay(1);
    }
}

/**
 * @brief Predictively preloads vector tiles while the render task is idle.
 *
 * @details Runs only when the pending queue is empty (no active render). On ESP32-P4
 *          it loads the leading border of the next grid (Chebyshev radius
 *          tilesGrid/2 + 1), ordered by dot product with the movement direction (GPS
 *          heading while following, drag velocity otherwise), so a tile crossing or a
 *          90 degree turn does not stall on an SD read. Loaded tiles stay pinned for
 *          PREFETCH_PIN_FRAMES render cycles. On the other targets the single-tile
 *          heading prefetch is kept unchanged.
 */
void Maps::prefetchNextTile()
{
    if (!mapSet.vectorMap)
        return;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    float dirTileX;
    float dirTileY;
    uint32_t centerTileX;
    uint32_t centerTileY;

    if (Maps::followGps)
    {
        const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
        if (gpsSnap.speed < PREFETCH_MIN_SPEED_KMH)
            return;
        const float rad = gpsSnap.heading * (float)M_PI / 180.0f;
        dirTileX = sinf(rad);
        dirTileY = -cosf(rad);
        centerTileX = lon2tilex(gpsSnap.longitude, zoomLevel);
        centerTileY = lat2tiley(gpsSnap.latitude, zoomLevel);
    }
    else
    {
        if (fabsf(velocityX) < PREFETCH_MIN_DRAG_VELOCITY && fabsf(velocityY) < PREFETCH_MIN_DRAG_VELOCITY)
            return;
        dirTileX = velocityX;
        dirTileY = velocityY;
        centerTileX = currentMapTile.tilex;
        centerTileY = currentMapTile.tiley;
    }

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return;

    const int16_t radius = (int16_t)tilesGrid / 2 + 1;
    struct Neighbor
    {
        int16_t dx;
        int16_t dy;
        float dot;
    };
    Neighbor ring[64];
    uint8_t n = 0;
    for (int16_t dy = -radius; dy <= radius; dy++)
    {
        for (int16_t dx = -radius; dx <= radius; dx++)
        {
            if (std::abs(dx) < radius && std::abs(dy) < radius)
                continue;
            if (dx == 0 && dy == 0)
                continue;
            const float dot = (float)dx * dirTileX + (float)dy * dirTileY;
            if (dot < 0.0f)
                continue;
            ring[n].dx = dx;
            ring[n].dy = dy;
            ring[n].dot = dot;
            n++;
        }
    }
    std::sort(ring, ring + n, [](const Neighbor& a, const Neighbor& b) { return a.dot > b.dot; });

    uint8_t loadedInPass = 0;
    for (uint8_t i = 0; i < n && loadedInPass < PREFETCH_MAX_LOAD_PER_PASS; i++)
    {
        const uint32_t targetX = centerTileX + ring[i].dx;
        const uint32_t targetY = centerTileY + ring[i].dy;
        const uint32_t targetHash = (uint32_t(zoomLevel) << 28) | (uint32_t(targetX & 0x3FFF) << 14) | uint32_t(targetY & 0x3FFF);

        bool present = false;
        for (const auto& entry : vectorCache)
        {
            if (entry.tileHash == targetHash)
            {
                present = true;
                break;
            }
        }
        if (present)
            continue;

        size_t dataSize = 0;
        if (!vectorCacheLookupOrLoad(targetX, targetY, zoomLevel, dataSize))
            continue;

        for (auto& entry : vectorCache)
        {
            if (entry.tileHash == targetHash)
            {
                entry.pinLeft = PREFETCH_PIN_FRAMES;
                break;
            }
        }
        loadedInPass++;
    }

    xSemaphoreGiveRecursive(mapMutex);
#else
    if (!Maps::followGps)
        return;

    const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
    if (gpsSnap.speed < PREFETCH_MIN_SPEED_KMH)
        return;

    const uint32_t centerTileX = lon2tilex(gpsSnap.longitude, zoomLevel);
    const uint32_t centerTileY = lat2tiley(gpsSnap.latitude, zoomLevel);

    const uint8_t sector = (uint8_t)(((gpsSnap.heading + 22) % 360) / 45);
    static const int8_t dirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int8_t dirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    const uint32_t targetX = centerTileX + dirX[sector];
    const uint32_t targetY = centerTileY + dirY[sector];

    const uint32_t targetHash = (uint32_t(zoomLevel) << 28) | (uint32_t(targetX & 0x3FFF) << 14) | uint32_t(targetY & 0x3FFF);
    if (targetHash == lastPrefetchHash)
        return;

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return;

    size_t dataSize = 0;
    vectorCacheLookupOrLoad(targetX, targetY, zoomLevel, dataSize);
    lastPrefetchHash = targetHash;

    for (auto& entry : vectorCache)
    {
        if (entry.tileHash == targetHash)
        {
            entry.isPinned = false;
            break;
        }
    }

    xSemaphoreGiveRecursive(mapMutex);
#endif
}

/**
 * @brief Predictively decodes the next PNG tile into the staging sprite.
 *
 * @details Runs only when the render queue is empty. Stages the leading edge tile of
 *          the next grid (Chebyshev radius tilesGrid/2 + 1) in the movement direction
 *          (GPS heading while following, drag velocity otherwise) so the next scroll
 *          crossing can blit it instead of decoding from SD.
 */
void Maps::prefetchPngTile()
{
    if (mapSet.vectorMap)
        return;
    if (pngStagingSprite.getBuffer() == nullptr)
        return;

    float dirTileX = 0.0f;
    float dirTileY = 0.0f;
    uint32_t centerTileX;
    uint32_t centerTileY;

    if (Maps::followGps)
    {
        const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
        if (gpsSnap.speed < PREFETCH_MIN_SPEED_KMH)
            return;
        const float rad = gpsSnap.heading * (float)M_PI / 180.0f;
        dirTileX = sinf(rad);
        dirTileY = -cosf(rad);
        centerTileX = lon2tilex(gpsSnap.longitude, zoomLevel);
        centerTileY = lat2tiley(gpsSnap.latitude, zoomLevel);
    }
    else
    {
        if (fabsf(velocityX) < PREFETCH_MIN_DRAG_VELOCITY && fabsf(velocityY) < PREFETCH_MIN_DRAG_VELOCITY)
            return;
        dirTileX = velocityX;
        dirTileY = velocityY;
        centerTileX = currentMapTile.tilex;
        centerTileY = currentMapTile.tiley;
    }

    const int32_t radius = (int32_t)tilesGrid / 2 + 1;
    const int32_t stepX = (dirTileX > 0.0f) ? radius : ((dirTileX < 0.0f) ? -radius : 0);
    const int32_t stepY = (dirTileY > 0.0f) ? radius : ((dirTileY < 0.0f) ? -radius : 0);
    if (stepX == 0 && stepY == 0)
        return;

    const uint32_t maxIdx = (uint32_t(1u) << zoomLevel) - 1u;
    const int64_t txi = (int64_t)centerTileX + stepX;
    const int64_t tyi = (int64_t)centerTileY + stepY;
    const uint32_t targetX = (uint32_t)std::min<int64_t>(std::max<int64_t>(txi, 0), maxIdx);
    const uint32_t targetY = (uint32_t)std::min<int64_t>(std::max<int64_t>(tyi, 0), maxIdx);

    const uint32_t targetHash = (uint32_t(zoomLevel) << 28) | (uint32_t(targetX & 0x3FFF) << 14) | uint32_t(targetY & 0x3FFF);
    if (pngStagingValid && pngStagedHash == targetHash)
        return;

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return;

    if (pngStagingValid && pngStagedHash == targetHash)
    {
        xSemaphoreGiveRecursive(mapMutex);
        return;
    }

    pngStagingValid = false;
    pngStagedHash = 0;

    char tilePath[128];
    snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoomLevel, targetX, targetY);

    int16_t stageX = 0;
    int16_t stageY = 0;
    if (pngStagingSprite.drawPngFile(tilePath, stageX, stageY))
    {
        pngStagedHash = targetHash;
        pngStagingValid = true;
    }

    xSemaphoreGiveRecursive(mapMutex);
}

/**
 * @brief Blits the staged PNG tile if it matches the requested tile hash.
 *
 * @details Consumes the staging buffer so the next crossing re-stages. Returns false
 *          when there is no valid staged tile for this position; the caller then falls
 *          back to a synchronous PNG decode.
 */
bool Maps::tryApplyStagedPng(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map)
{
    if (!pngStagingValid)
    {
        return false;
    }

    const uint32_t hash = (uint32_t(zoom) << 28) | (uint32_t(tileX & 0x3FFF) << 14) | uint32_t(tileY & 0x3FFF);
    if (hash != pngStagedHash)
    {
        return false;
    }

    map.pushImage(screenX, screenY, mapTileSize, mapTileSize, static_cast<uint16_t*>(pngStagingSprite.getBuffer()));
    pngStagingValid = false;
    pngStagedHash = 0;
    return true;
}

/**
 * @brief Render a single PNG tile
 * 
 * @param tileX X Tile
 * @param tileY Y Tile
 * @param zoom Zoom level
 * @param screenX X PNG position on sprite
 * @param screenY Y PNG position on sprite
 * @param map Map sprite
 */
void Maps::renderPngTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map)
{
    if (tryApplyStagedPng(tileX, tileY, zoom, screenX, screenY, map))
        return;
    char tilePath[128];
    snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoom, tileX, tileY);
    if (!map.drawPngFile(tilePath, screenX, screenY))
    {
        map.fillRect(screenX, screenY, mapTileSize, mapTileSize, TFT_BLACK);
        map.drawPngFile(noMapFile, screenX + mapTileSize / 2 - 50, screenY + mapTileSize / 2 - 50);
    }
}

/**
 * @brief Display the map on screen with rotation and dynamic cropping.
 */
void Maps::displayMap()
{
    if (!Maps::isMapFound)
    {
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0);
        return;
    }

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(50)) != pdTRUE)
        return;

    mapCanvasParent()->startWrite();

    if (Maps::followGps)
    {
        const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();

        uint16_t mapHeading = 0;
        #ifdef ENABLE_COMPASS
        {
            int sensorHeading = 0;
            if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(5)) == pdTRUE)
            {
                sensorHeading = globalSensorData.heading;
                xSemaphoreGive(sensorMutex);
            }
            mapHeading = mapSet.mapRotationComp ? (uint16_t)sensorHeading : gpsSnap.heading;
        }
        #else
            mapHeading = gpsSnap.heading;
        #endif

        const float lat = gpsSnap.latitude;
        const float lon = gpsSnap.longitude;
        const int8_t gridOffset = tilesGrid / 2;
        Maps::navArrowPosition = Maps::coord2ScreenPos(lon, lat, Maps::zoomLevel, Maps::mapTileSize);

        // Hysteresis: skip redraw if nothing changed
        if (!Maps::redrawMap)
        {
            if (mapHeading == lastRenderedHeading &&
                navArrowPosition.posX == lastRenderedArrowPos.posX &&
                navArrowPosition.posY == lastRenderedArrowPos.posY)
            {
                mapCanvasParent()->endWrite();
                xSemaphoreGiveRecursive(mapMutex);
                return;
            }
        }
        lastRenderedHeading = mapHeading;
        lastRenderedArrowPos = navArrowPosition;

        if (use3DCache)
            // 3D mode: scanline perspective transform with heading baked in
            apply3DPerspective(mapHeading);
        else
        {
            Maps::mapTempSprite.setPivot(gridOffset * mapTileSize + Maps::navArrowPosition.posX,
                                         gridOffset * mapTileSize + Maps::navArrowPosition.posY);
            Maps::mapSprite.setPivot(mapScrWidth / 2, mapScrHeight / 2);
            Maps::mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
        }
    }
    else
    {
        if (!Maps::redrawMap)
        {
            if (displayOffsetX == lastRenderedDisplayOffsetX &&
                displayOffsetY == lastRenderedDisplayOffsetY &&
                manualHeading  == lastRenderedManualHeading)
            {
                mapCanvasParent()->endWrite();
                xSemaphoreGiveRecursive(mapMutex);
                return;
            }
        }
        lastRenderedDisplayOffsetX  = displayOffsetX;
        lastRenderedDisplayOffsetY  = displayOffsetY;
        lastRenderedManualHeading   = manualHeading;

        if (manualHeading != 0.0f)
        {
            int16_t pivX = tileWidth  / 2 + displayOffsetX;
            int16_t pivY = tileHeight / 2 + displayOffsetY;
            Maps::mapTempSprite.setPivot(pivX, pivY);
            Maps::mapSprite.setPivot(mapScrWidth / 2, mapScrHeight / 2);
            Maps::mapTempSprite.pushRotated(&mapSprite, 360.0f - manualHeading, TFT_TRANSPARENT);
        }
        else
        {
            int16_t cropX = (tileWidth  - mapScrWidth)  / 2 + displayOffsetX;
            int16_t cropY = (tileHeight - mapScrHeight) / 2 + displayOffsetY;
            mapTempSprite.pushSprite(&mapSprite, -cropX, -cropY);
        }
    }

    Maps::redrawMap = false;
    mapCanvasParent()->endWrite();
    xSemaphoreGiveRecursive(mapMutex);
}

/**
 * @brief Set waypoint coordinates
 * 
 * @param wptLat Waypoint latitude
 * @param wptLon Waypoint longitude
 */
void Maps::setWaypoint(float wptLat, float wptLon)
{
    Maps::destLat = wptLat;
    Maps::destLon = wptLon;
    Maps::hasWaypoint = true;
}

/**
 * @brief Mark map for redraw
 */
/**
 * @brief Returns true when there is an active navigation target (track or waypoint).
 */
bool Maps::isNavActive() const
{
    if (navCtx.trackData.size() > 0)
        return true;
    if (hasWaypoint)
        return true;
    return false;
}

void Maps::update3DCache()
{
    use3DCache = mapSet.map3D && mapSet.vectorMap && isNavActive() && !scrolling;
}

/**
 * @brief Applies a perspective scanline transform from mapTempSprite to mapSprite.
 *
 * @details Reads the 2D tile-space sprite (mapTempSprite), applies heading rotation and
 *          perspective projection in a single scanline pass, and writes directly into
 *          mapSprite (the viewport buffer). The GPS position is placed at the lower third
 *          of the viewport. For each output scanline Y, the corresponding ground-plane Y
 *          in the source sprite is computed via inverse perspective, then each pixel X is
 *          sampled by rotating back from heading-up screen space to tile space.
 *
 * @param heading  Current map heading in degrees (0 = north up, 90 = east up).
 */
void Maps::apply3DPerspective(uint16_t heading)
{
    uint16_t* src = static_cast<uint16_t*>(mapTempSprite.getBuffer());
    uint16_t* dst = static_cast<uint16_t*>(mapSprite.getBuffer());
    if (!src || !dst)
        return;

    const int srcW = (int)(mapTempSprite.bufferLength() / (tileHeight * 2));
    const int srcH = (int)tileHeight;
    const int dstW = (int)mapScrWidth;
    const int dstH = (int)mapScrHeight;
    const int dstStride = (int)(mapSprite.bufferLength() / (mapScrHeight * 2));

    // GPS position in tile-space (center of the tile grid)
    const int8_t gridOffset = tilesGrid / 2;
    const int gpsTileX = gridOffset * mapTileSize + (int)navArrowPosition.posX;
    const int gpsTileY = gridOffset * mapTileSize + (int)navArrowPosition.posY;

    // GPS lands at lower third of the viewport; shift up when climb overlay visible
    int gpsScreenY = dstH * 3 / 4;
    if (climbOverlay != NULL && !lv_obj_has_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN))
        gpsScreenY -= lv_obj_get_height(climbOverlay) / 2;

    // Heading rotation: rotate tile-space coords so heading points up
    const float headingRad = static_cast<float>(heading) * (static_cast<float>(M_PI) / 180.0f);
    const float cosH = cosf(headingRad);
    const float sinH = sinf(headingRad);

    // Perspective parameters: horizon at top quarter of screen
    const int horizonScreenY = dstH / 5;
    const float tiltRad = mapTilt * (static_cast<float>(M_PI) / 180.0f);
    const float invCosTilt = 1.0f / cosf(tiltRad);

    // Sky color: soft blue ~#A8C8E8 (byte-swapped for direct buffer write)
    const uint16_t skyColor = 0x5DAE;

    const float halfW = static_cast<float>(dstW) * 0.5f;
    const float invSpan = 1.0f / static_cast<float>(gpsScreenY - horizonScreenY);

    for (int y = 0; y < dstH; y++)
    {
        uint16_t* dstRow = dst + y * dstStride;

        if (y <= horizonScreenY)
        {
            for (int x = 0; x < dstW; x++)
                dstRow[x] = skyColor;
            continue;
        }

        // t=0 at horizon, t=1 at GPS screen position
        float t = static_cast<float>(y - horizonScreenY) * invSpan;
        if (t <= 0.0f)
            continue;

        float scale = t * invCosTilt;
        float invScale = 1.0f / scale;

        // Positive srcRelY = ahead (up in heading-up view = forward direction)
        float srcRelY = (focalLength * invScale) * (1.0f - t) / t;

        // sx/sy are linear in x; accumulate in Q16 fixed point so the inner loop
        // drops the per-pixel division, multiplies and float->int conversions.
        const float dsxF = invScale * cosH;
        const float dsyF = invScale * sinH;

        const float sxStart = static_cast<float>(gpsTileX) + (-halfW * invScale) * cosH + srcRelY * sinH;
        const float syStart = static_cast<float>(gpsTileY) - ((halfW * invScale) * sinH + srcRelY * cosH);

        const float CLAMP = 30000.0f;
        float sxC = fmaxf(-CLAMP, fminf(CLAMP, sxStart));
        float syC = fmaxf(-CLAMP, fminf(CLAMP, syStart));

        int32_t sxFix = static_cast<int32_t>(sxC * 65536.0f);
        int32_t syFix = static_cast<int32_t>(syC * 65536.0f);
        const int32_t dsxFix = static_cast<int32_t>(dsxF * 65536.0f);
        const int32_t dsyFix = static_cast<int32_t>(dsyF * 65536.0f);

        for (int x = 0; x < dstW; x++)
        {
            int sx = sxFix >> 16;
            int sy = syFix >> 16;

            if (sx >= 0 && sx < srcW && sy >= 0 && sy < srcH)
                dstRow[x] = src[sy * srcW + sx];
            else
                dstRow[x] = skyColor;

            sxFix += dsxFix;
            syFix += dsyFix;
        }
    }
}

void Maps::updateMap()
{
    Maps::oldMapTile = {};
    vectorNeedsRender = true;
    update3DCache();
}

/**
 * @brief Panning map by tile offsets
 * 
 * @param dx X scroll offset
 * @param dy Y scroll offset
 */
void Maps::panMap(int8_t dx, int8_t dy)
{
    Maps::currentMapTile.tilex += dx;
    Maps::currentMapTile.tiley += dy;
    Maps::currentMapTile.lon = Maps::tilex2lon(Maps::currentMapTile.tilex, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = Maps::tiley2lat(Maps::currentMapTile.tiley, Maps::currentMapTile.zoom);
}

/**
 * @brief Center map on GPS position
 * 
 * @param lat GPS Latitude
 * @param lon GPS Longitude
 */
void Maps::centerOnGps(float lat, float lon)
{
    Maps::followGps = true;
    scrolling = false;
    Maps::currentMapTile.zoom = Maps::zoomLevel;
    Maps::currentMapTile.tilex = Maps::lon2tilex(lon, Maps::currentMapTile.zoom);
    Maps::currentMapTile.tiley = Maps::lat2tiley(lat, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = lat;
    Maps::currentMapTile.lon = lon;
    manualHeading = 0.0f;
    lastRenderedManualHeading = 0.0f;
    resetScrollState();
}

/**
 * @brief Reset all scroll offsets
 */
void Maps::resetScrollState()
{
    tileX = 0;
    tileY = 0;
    lastTileX = 0;
    lastTileY = 0;
    offsetX = 0;
    offsetY = 0;
    displayOffsetX = 0;
    displayOffsetY = 0;
    pendingDx = 0;
    pendingDy = 0;
    velocityX = 0;
    velocityY = 0;
    vectorDeferred = false;
    vectorPending = false;
    vectorSteps.clear();
}

/**
 * @brief Smooth scroll the map
 * 
 * @param dx X scroll offset
 * @param dy Y scroll offset
 */
void Maps::scrollMap(int16_t dx, int16_t dy)
{
    if (manualHeading != 0.0f)
    {
        float rad = manualHeading * (float)M_PI / 180.0f;
        float c = cosf(rad);
        float s = sinf(rad);
        float dxR = (float)dx * c - (float)dy * s;
        float dyR = (float)dx * s + (float)dy * c;
        dx = (int16_t)dxR;
        dy = (int16_t)dyR;
    }
    pendingDx += dx;
    pendingDy += dy;
    scrolling = true;

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        return;
    }

    dx = pendingDx;
    dy = pendingDy;
    pendingDx = 0;
    pendingDy = 0;

    if (dx != 0 || dy != 0)
    {
        const int16_t threshold = 128;
        // Elastic factor: reduces movement as we approach or exceed the threshold
        float factorX = 1.0f;
        float factorY = 1.0f;
        
        if (abs(Maps::offsetX) > threshold / 2)
            factorX = 1.0f - (float)abs(Maps::offsetX) / (float)tileWidth;
        if (abs(Maps::offsetY) > threshold / 2)
            factorY = 1.0f - (float)abs(Maps::offsetY) / (float)tileHeight;

        Maps::offsetX += (int16_t)((float)dx * factorX);
        Maps::offsetY += (int16_t)((float)dy * factorY);
        Maps::followGps = false;
    }

    const int16_t maxOffsetX = (tileWidth - mapScrWidth) / 2 - 5;
    const int16_t maxOffsetY = (tileHeight - mapScrHeight) / 2 - 5;

    if (Maps::offsetX > maxOffsetX)
        Maps::offsetX = maxOffsetX;
    if (Maps::offsetX < -maxOffsetX)
        Maps::offsetX = -maxOffsetX;
    if (Maps::offsetY > maxOffsetY)
        Maps::offsetY = maxOffsetY;
    if (Maps::offsetY < -maxOffsetY)
        Maps::offsetY = -maxOffsetY;

    scrollUpdated = false;
    #ifdef T4_S3
        const int16_t threshold = 160;
    #else
        const int16_t threshold = 128;
    #endif
    const int16_t tileSize = Maps::mapTileSize;

    while (Maps::offsetX <= -threshold)
    {
        tileX--;
        Maps::offsetX += tileSize;
        scrollUpdated = true;
    }
    while (Maps::offsetX >= threshold)
    {
        tileX++;
        Maps::offsetX -= tileSize;
        scrollUpdated = true;
    }

    while (Maps::offsetY <= -threshold)
    {
        tileY--;
        Maps::offsetY += tileSize;
        scrollUpdated = true;
    }
    while (Maps::offsetY >= threshold)
    {
        tileY++;
        Maps::offsetY -= tileSize;
        scrollUpdated = true;
    }

    if (scrollUpdated)
    {
        const int8_t deltaTileX = tileX - lastTileX;
        const int8_t deltaTileY = tileY - lastTileY;
        Maps::panMap(deltaTileX, deltaTileY);

        if (!mapSet.vectorMap)
        {
            const bool singleStep = (abs(deltaTileX) == 1 && deltaTileY == 0) ||
                                    (abs(deltaTileY) == 1 && deltaTileX == 0);
            if (singleStep && pendingTiles.empty())
                Maps::preloadTiles(deltaTileX, deltaTileY);
            else
                generateMap(zoomLevel);
        }
        else
        {
            const bool axial = (deltaTileX == 0) != (deltaTileY == 0);
            const int8_t stepDirX = deltaTileX > 0 ? 1 : (deltaTileX < 0 ? -1 : 0);
            const int8_t stepDirY = deltaTileY > 0 ? 1 : (deltaTileY < 0 ? -1 : 0);
            const uint8_t stepCount = (uint8_t)(abs(deltaTileX) + abs(deltaTileY));
            const bool sequenceBusy = vectorPending ||
                                       !pendingTiles.empty() ||
                                       !vectorSteps.empty();
            const bool sameDirection = !sequenceBusy ||
                                       (vectorDirX == stepDirX &&
                                        vectorDirY == stepDirY);
            if (axial && stepCount > 0 && !inertia && !followGps && sameDirection)
            {
                Maps::preloadVectorTiles(stepDirX, stepDirY, stepCount);
            }
            else
            {
                const int16_t marginX = (tileWidth - mapScrWidth) / 2 - threshold;
                const int16_t marginY = (tileHeight - mapScrHeight) / 2 - threshold;
                const int16_t deferredX = offsetX + (tileX - lastTileX) * mapTileSize;
                const int16_t deferredY = offsetY + (tileY - lastTileY) * mapTileSize;
                const bool exceedsMargin = abs(deferredX) > marginX || abs(deferredY) > marginY;

                // During inertia the re-render is postponed even past the grid margin (the stale
                // edge is tolerated until the flick stops) so the grid is not re-rasterized on every
                // crossing. A finger drag is slow enough to re-render when the margin is exceeded.
                if (scrolling && (inertia || !exceedsMargin))
                    vectorDeferred = true;
                else
                {
                    vectorPending = false;
                    vectorSteps.clear();
                    pendingTiles.clear();
                    pendingTilesNotEmpty = false;
                    vectorDeferred = false;
                    updateMap();
                    generateMap(zoomLevel);
                }
            }
        }

        Maps::redrawMap = true;
    }

    if (pendingTiles.empty() && vectorSteps.empty() && !vectorDeferred)
    {
        displayOffsetX = offsetX;
        displayOffsetY = offsetY;
        lastTileX = tileX;
        lastTileY = tileY;
    }
    else
    {
        // When rendering is pending (after a swap) or deferred, we stay at the virtual relative
        // position to avoid jumping until the new grid is complete.
        displayOffsetX = offsetX + (tileX - lastTileX) * mapTileSize;
        displayOffsetY = offsetY + (tileY - lastTileY) * mapTileSize;
    }

    xSemaphoreGiveRecursive(mapMutex);
}

/**
 * @brief Flush a deferred vector re-render once the gesture settles.
 *
 * @details During an active drag or inertia, scrollMap() postpones the vector grid re-render and
 *          keeps showing the already rasterized sprite shifted via displayOffset. This consolidates
 *          the pending re-render at the new center once the movement stops (release or inertia end).
 */
void Maps::commitScroll()
{
    scrolling = false;

    if (!vectorDeferred)
        return;

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return;

    if (vectorPending || !vectorSteps.empty())
    {
        vectorPending = false;
        vectorSteps.clear();
        pendingTiles.clear();
        pendingTilesNotEmpty = false;
    }
    vectorDeferred = false;
    updateMap();
    generateMap(zoomLevel);
    Maps::redrawMap = true;

    xSemaphoreGiveRecursive(mapMutex);
}

/**
 * @brief Shifts the vector map sprite by one tile in one axis.
 *
 * @details On the ESP32-P4 the carry-over region is copied out-of-place into a
 *          second PSRAM buffer with a single non-blocking PPA SRM transaction
 *          (no overlap, so it is safe). The copy runs on the PPA's own DMA while
 *          the render task decodes the incoming band, so the transfer is hidden
 *          behind the CPU-bound work instead of adding to the step. The vacated
 *          border is cleared with a PPA fill, the sprite is switched to the
 *          second buffer, and the caller waits for the SRM before painting over
 *          the copied region. Falls back to the LGFX scroll on the other targets
 *          or whenever the second buffer or the SRM client is unavailable.
 *
 * @param shiftX Pixel shift in X (0 for a vertical scroll).
 * @param shiftY Pixel shift in Y (0 for a horizontal scroll).
 */
void Maps::scrollVectorSprite(int16_t shiftX, int16_t shiftY)
{
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    uint8_t* cur = static_cast<uint8_t*>(mapTempSprite.getBuffer());
    uint8_t* dst = nullptr;
    if (mapTempBufs[0] && mapTempBufs[1])
    {
        if (cur == mapTempBufs[0])
            dst = mapTempBufs[1];
        else if (cur == mapTempBufs[1])
            dst = mapTempBufs[0];
    }
    if (dst && ppaSrmClient)
    {
        // Never touch the inactive buffer while a previous copy is still running.
        while (srmInFlight)
            vTaskDelay(1);

        const int16_t w = (int16_t)tileWidth;
        const int16_t h = (int16_t)tileHeight;
        const int16_t ts = (int16_t)mapTileSize;
        int16_t srcX = 0, srcY = 0, dstX = 0, dstY = 0;
        uint32_t copyW = 0, copyH = 0;
        if (shiftX != 0)
        {
            copyW = (uint32_t)(w - abs(shiftX));
            copyH = (uint32_t)h;
            srcX = (shiftX < 0) ? (int16_t)(-shiftX) : 0;
            dstX = (shiftX < 0) ? 0 : shiftX;
        }
        else
        {
            copyH = (uint32_t)(h - abs(shiftY));
            copyW = (uint32_t)w;
            srcY = (shiftY < 0) ? (int16_t)(-shiftY) : 0;
            dstY = (shiftY < 0) ? 0 : shiftY;
        }

        ppa_srm_oper_config_t srm = {};
        srm.in.buffer = cur;
        srm.in.pic_w = (uint32_t)w;
        srm.in.pic_h = (uint32_t)h;
        srm.in.block_w = copyW;
        srm.in.block_h = copyH;
        srm.in.block_offset_x = (uint32_t)srcX;
        srm.in.block_offset_y = (uint32_t)srcY;
        srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
        srm.out.buffer = dst;
        srm.out.buffer_size = (uint32_t)w * (uint32_t)h * 2;
        srm.out.pic_w = (uint32_t)w;
        srm.out.pic_h = (uint32_t)h;
        srm.out.block_offset_x = (uint32_t)dstX;
        srm.out.block_offset_y = (uint32_t)dstY;
        srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
        srm.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
        srm.scale_x = 1.0f;
        srm.scale_y = 1.0f;
        srm.mode = PPA_TRANS_MODE_NON_BLOCKING;
        srm.user_data = (void*)&srmInFlight;
        srmInFlight = true;
        if (ppa_do_scale_rotate_mirror(ppaSrmClient, &srm) == ESP_OK)
        {
            // Discard stale CPU-cached lines in the vacated band only (the SRM
            // driver already invalidates its own output window) so neither the DMA
            // fill nor the SRM output gets clobbered by old cached content being
            // written back afterwards.
            if (shiftX != 0)
            {
                const uint32_t bandX = (shiftX < 0) ? (uint32_t)(w - ts) : 0;
                for (uint32_t y = 0; y < (uint32_t)h; y++)
                    esp_cache_msync(dst + y * (uint32_t)w * 2 + bandX * 2, (uint32_t)ts * 2,
                                    ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
            }
            else
            {
                const uint32_t bandY = (shiftY < 0) ? (uint32_t)(h - ts) : 0;
                esp_cache_msync(dst + bandY * (uint32_t)w * 2, (uint32_t)ts * (uint32_t)w * 2,
                                ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
            }

            // Clear the vacated border before the incoming band is painted on it.
            ppa_fill_oper_config_t fill = {};
            fill.fill_argb_color.val = rgb565_to_argb8888(0xF7BE);
            fill.out.buffer = dst;
            fill.out.buffer_size = (uint32_t)w * (uint32_t)h * 2;
            fill.out.pic_w = (uint32_t)w;
            fill.out.pic_h = (uint32_t)h;
            fill.out.block_offset_x = (shiftX != 0) ? ((shiftX < 0) ? (uint32_t)(w - ts) : 0) : 0;
            fill.out.block_offset_y = (shiftY != 0) ? ((shiftY < 0) ? (uint32_t)(h - ts) : 0) : 0;
            fill.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;
            fill.fill_block_w = (shiftX != 0) ? (uint32_t)ts : (uint32_t)w;
            fill.fill_block_h = (shiftY != 0) ? (uint32_t)ts : (uint32_t)h;
            fill.mode = PPA_TRANS_MODE_BLOCKING;
            ppa_do_fill(ppaFillClient, &fill);
            esp_cache_msync(dst, (uint32_t)w * (uint32_t)h * 2,
                            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);

            mapTempSprite.setBuffer(dst, w, h);
            return;
        }
        srmInFlight = false;
    }
#endif
    if (shiftX != 0)
        mapTempSprite.scroll(shiftX, 0);
    else
        mapTempSprite.scroll(0, shiftY);
}

/**
 * @brief Queue one incoming vector-map border for incremental rendering.
 *
 * @param centerTileIdxX Center tile X after the incremental step.
 * @param centerTileIdxY Center tile Y after the incremental step.
 * @param dirX X pan direction (-1, 0, +1).
 * @param dirY Y pan direction (-1, 0, +1).
 */
void Maps::queueVectorStep(uint32_t centerTileIdxX, uint32_t centerTileIdxY,
                           int8_t dirX, int8_t dirY)
{
    const int16_t tileSize = mapTileSize;
    const int8_t gridOffset = tilesGrid / 2;
    const int32_t tlX = (int32_t)centerTileIdxX - gridOffset;
    const int32_t tlY = (int32_t)centerTileIdxY - gridOffset;
    VectorStep step;
    step.dirX = dirX;
    step.dirY = dirY;

    if (dirX != 0)
    {
        const int gx = (dirX > 0) ? (tilesGrid - 1) : 0;
        const uint32_t tx = (uint32_t)(tlX + gx);
        for (int gy = 0; gy < tilesGrid; gy++)
            step.tiles.push_back({tx, (uint32_t)(tlY + gy),
                                  (int16_t)(gx * tileSize), (int16_t)(gy * tileSize), TILE_NAV});
    }
    else
    {
        const int gy = (dirY > 0) ? (tilesGrid - 1) : 0;
        const uint32_t ty = (uint32_t)(tlY + gy);
        for (int gx = 0; gx < tilesGrid; gx++)
            step.tiles.push_back({(uint32_t)(tlX + gx), ty,
                                  (int16_t)(gx * tileSize), (int16_t)(gy * tileSize), TILE_NAV});
    }

    vectorSteps.push_back(step);
}

/**
 * @brief Queue one or more axial vector-map steps.
 *
 * @details Keeps the already rasterized map pixels and queues only the incoming border of each
 *          step. The same incremental path is shared by P4 and S3.
 *
 * @param dirX X pan direction (-1, 0, +1).
 * @param dirY Y pan direction (-1, 0, +1).
 * @param stepCount Number of consecutive steps to queue.
 */
void Maps::preloadVectorTiles(int8_t dirX, int8_t dirY, uint8_t stepCount)
{
    const int8_t gridOffset = tilesGrid / 2;
    const uint32_t centerTileIdxX = Maps::currentMapTile.tilex;
    const uint32_t centerTileIdxY = Maps::currentMapTile.tiley;
    const int32_t tlX = (int32_t)centerTileIdxX - gridOffset;
    const int32_t tlY = (int32_t)centerTileIdxY - gridOffset;
    mapTlX = (float)tlX;
    mapTlY = (float)tlY;
    Maps::oldMapTile.tilex = centerTileIdxX;
    Maps::oldMapTile.tiley = centerTileIdxY;
    Maps::oldMapTile.zoom  = Maps::zoomLevel;

    Maps::totalBounds = getTileBounds((uint32_t)tlX, (uint32_t)tlY, Maps::zoomLevel);
    const tileBounds brBounds = getTileBounds((uint32_t)(tlX + tilesGrid - 1),
                                               (uint32_t)(tlY + tilesGrid - 1), Maps::zoomLevel);
    if (brBounds.lat_min < totalBounds.lat_min)
        totalBounds.lat_min = brBounds.lat_min;
    if (brBounds.lat_max > totalBounds.lat_max)
        totalBounds.lat_max = brBounds.lat_max;
    if (brBounds.lon_min < totalBounds.lon_min)
        totalBounds.lon_min = brBounds.lon_min;
    if (brBounds.lon_max > totalBounds.lon_max)
        totalBounds.lon_max = brBounds.lon_max;

    if (Maps::isMapFound && Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
        Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &wptPosX, &wptPosY);
    else
    {
        Maps::wptPosX = -1;
        Maps::wptPosY = -1;
    }

    const int32_t firstCenterX = (int32_t)centerTileIdxX - dirX * stepCount;
    const int32_t firstCenterY = (int32_t)centerTileIdxY - dirY * stepCount;
    for (uint8_t step = 1; step <= stepCount; step++)
    {
        const uint32_t stepCenterX = (uint32_t)(firstCenterX + dirX * step);
        const uint32_t stepCenterY = (uint32_t)(firstCenterY + dirY * step);
        queueVectorStep(stepCenterX, stepCenterY, dirX, dirY);
    }

    vectorDirX = dirX;
    vectorDirY = dirY;
    vectorPending = true;
    pendingTilesNotEmpty = true;
    vectorDeferred = false;
    redrawMap = true;
    xEventGroupClearBits(mapEventGroup, MAP_EVENT_DONE | MAP_EVENT_ERROR);
    xEventGroupSetBits(mapEventGroup, MAP_EVENT_START);
}

/**
 * @brief Incrementally scroll the PNG grid by one tile row or column.
 *
 * @details Shifts the already rendered sprite one tile in the pan direction and loads only the
 *          incoming edge tiles from SD instead of re-reading the whole grid. Advances the grid
 *          anchor (mapTlX/mapTlY, oldMapTile) to the new top-left, recomputes totalBounds
 *          from the grid corners, repositions the waypoint and redraws the track.
 *
 * @param dirX X pan direction (-1, 0, +1).
 * @param dirY Y pan direction (-1, 0, +1).
 */
void Maps::preloadTiles(int8_t dirX, int8_t dirY)
{
    const int16_t tileSize = mapTileSize;
    const int8_t gridOffset = tilesGrid / 2;

    const uint32_t centerTileIdxX = Maps::currentMapTile.tilex;
    const uint32_t centerTileIdxY = Maps::currentMapTile.tiley;
    const int32_t tlX = (int32_t)centerTileIdxX - gridOffset;
    const int32_t tlY = (int32_t)centerTileIdxY - gridOffset;

    if (dirX != 0)
        mapTempSprite.scroll(-dirX * tileSize, 0);
    else if (dirY != 0)
        mapTempSprite.scroll(0, -dirY * tileSize);

    mapTlX = (float)tlX;
    mapTlY = (float)tlY;
    Maps::oldMapTile.tilex = centerTileIdxX;
    Maps::oldMapTile.tiley = centerTileIdxY;
    Maps::oldMapTile.zoom  = Maps::zoomLevel;

    bool centerFound = false;
    if (dirX != 0)
    {
        const int gx = (dirX > 0) ? (tilesGrid - 1) : 0;
        for (int gy = 0; gy < tilesGrid; gy++)
            loadPngTileIntoSprite(tlX, tlY, gx, gy, centerTileIdxX, centerTileIdxY, Maps::zoomLevel, centerFound);
    }
    else if (dirY != 0)
    {
        const int gy = (dirY > 0) ? (tilesGrid - 1) : 0;
        for (int gx = 0; gx < tilesGrid; gx++)
            loadPngTileIntoSprite(tlX, tlY, gx, gy, centerTileIdxX, centerTileIdxY, Maps::zoomLevel, centerFound);
    }
    (void)centerFound;

    Maps::totalBounds = getTileBounds((uint32_t)tlX, (uint32_t)tlY, Maps::zoomLevel);
    const tileBounds brBounds = getTileBounds((uint32_t)(tlX + tilesGrid - 1), (uint32_t)(tlY + tilesGrid - 1), Maps::zoomLevel);
    if (brBounds.lat_min < totalBounds.lat_min)
        totalBounds.lat_min = brBounds.lat_min;
    if (brBounds.lat_max > totalBounds.lat_max)
        totalBounds.lat_max = brBounds.lat_max;
    if (brBounds.lon_min < totalBounds.lon_min)
        totalBounds.lon_min = brBounds.lon_min;
    if (brBounds.lon_max > totalBounds.lon_max)
        totalBounds.lon_max = brBounds.lon_max;

    if (Maps::isMapFound && Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
        Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &wptPosX, &wptPosY);
    else
    {
        Maps::wptPosX = -1;
        Maps::wptPosY = -1;
    }

    drawTrack(mapTempSprite);
    drawWaypoint(mapTempSprite);
    redrawMap = true;
    xEventGroupSetBits(mapEventGroup, MAP_EVENT_DONE);
}

/**
 * @brief Returns the LOD (Level of Detail) skip threshold in pixels for the given zoom level.
 *
 * @details Used by renderVectorLine() and renderVectorPolygon() to skip coordinate pairs
 *          that are too close together to be visually relevant at the current zoom. While the
 *          renderer is behind the finger during a fast drag (aggressiveLod) the threshold is
 *          roughly doubled so fewer vertices are painted.
 *
 * @param zoom Current map zoom level.
 * @return Pixel distance threshold below which coordinates are skipped.
 */
static int16_t getLODThreshold(uint8_t zoom)
{
    const int16_t base = (zoom <= 12) ? 3 : (zoom <= 14) ? 2 : 1;
    return aggressiveLod ? base * 2 + 1 : base;
}

static uint32_t getPolygonAreaCullThreshold(uint8_t zoom)
{
    if (zoom <= 12)
        return 256;
    if (zoom <= 15)
        return 64;
    return 0;
}

/**
 * @brief Darken a color
 * 
 * @param color 
 * @param amount Dark amount
 * @return uint16_t Darken color
 */
uint16_t Maps::darkenRGB565(const uint16_t color, const float amount)
{
    // Integer-only fast path: precomputed factor avoids float in hot path
    static uint16_t lastInColor = 0;
    static uint16_t lastFactor = 0;
    static uint16_t lastOutColor = 0;

    uint16_t factor = (uint16_t)((int)((1.0f - amount) * 256.0f));

    if (color == lastInColor && factor == lastFactor)
        return lastOutColor;

    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    r = (uint8_t)((r * factor) >> 8);
    g = (uint8_t)((g * factor) >> 8);
    b = (uint8_t)((b * factor) >> 8);
    
    lastInColor = color;
    lastFactor = factor;
    lastOutColor = ((r << 11) | (g << 5) | b);
    return lastOutColor;
}

/**
 * @brief Fills a polygon (including shapes with holes/rings) using the Scanline AEL algorithm.
 * 
 * @details This function implements the Active Edge List (AEL) algorithm to rasterize convex, 
 *          concave, or complex polygons composed of multiple rings. It utilizes 16-bit 
 *          fixed-point arithmetic for edge slopes and sub-pixel X-coordinate precision 
 *          to ensure smooth transitions between scanlines.
 * 
 * @param map        Reference to the target MapCanvas where the polygon is rendered.
 * @param px         Array of X-coordinates for the vertices.
 * @param py         Array of Y-coordinates for the vertices.
 * @param numPoints  Total count of vertices across all rings.
 * @param color      16-bit (RGB565) color for the fill.
 * @param xOffset    Horizontal translation applied to the final drawing coordinates.
 * @param yOffset    Vertical translation applied to the final drawing coordinates.
 * @param ringCount  The number of independent rings (use 0 or 1 for simple polygons).
 * @param ringEnds   Array containing the end indices for each ring in the px/py arrays. 
 */
void Maps::fillPolygonGeneral(MapCanvas &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount, const uint16_t* ringEnds)
{
    if (numPoints < 3)
        return;

    uint16_t* buf = static_cast<uint16_t*>(map.getBuffer());
    uint32_t stride = 0;
    uint16_t rawColor = (color >> 8) | (color << 8);
    if (buf)
        stride = map.bufferLength() / (tileHeight * 2);
    else
        ESP_LOGE(TAG, "fillPoly FALLBACK buf=null");

    int minY = INT_MAX;
    int maxY = INT_MIN;
    for (int i = 0; i < numPoints; i++)
    {
        if (py[i] < minY)
            minY = py[i];
        if (py[i] > maxY)
            maxY = py[i];
    }

    if (maxY < 0 || minY >= (int)tileHeight)
        return;

    edgePool.clear();
    int clampedMaxY = std::min(maxY, (int)tileHeight - 1);
    int bucketCount = clampedMaxY - minY + 1;
    if ((int)edgeBuckets.size() < bucketCount)
        edgeBuckets.resize(bucketCount, -1);
    else
        std::fill(edgeBuckets.begin(), edgeBuckets.begin() + bucketCount, -1);
    uint16_t count = (ringCount == 0) ? 1 : ringCount;
    uint16_t defaultEnds[1] = { (uint16_t)numPoints };
    const uint16_t* ends = (ringEnds == nullptr) ? defaultEnds : ringEnds;
    int ringStart = 0;

    for (uint16_t r = 0; r < count; r++)
    {
        int ringEnd = ends[r];
        if (ringEnd > numPoints)
            ringEnd = numPoints;
        int ringNumPoints = ringEnd - ringStart;
        if (ringNumPoints < 3)
        {
            ringStart = ringEnd;
            continue;
        }
        for (int i = 0; i < ringNumPoints; i++)
        {
            int next = (i + 1) % ringNumPoints;
            int x1 = px[ringStart + i];
            int y1 = py[ringStart + i];
            int x2 = px[ringStart + next];
            int y2 = py[ringStart + next];
            if (y1 == y2)
                continue;
            int startBucketY = (y1 < y2) ? y1 : y2;
            if (startBucketY > clampedMaxY)
                continue;
            Edge e;
            e.nextActive = -1;
            if (y1 < y2)
            {
                e.yMax = y2;
                e.xVal = x1 << 16;
                e.slope = ((x2 - x1) << 16) / (y2 - y1);
                e.nextInBucket = edgeBuckets[y1 - minY];
                edgePool.push_back(e);
                edgeBuckets[y1 - minY] = edgePool.size() - 1;
            }
            else
            {
                e.yMax = y1;
                e.xVal = x2 << 16;
                e.slope = ((x1 - x2) << 16) / (y1 - y2);
                e.nextInBucket = edgeBuckets[y2 - minY];
                edgePool.push_back(e);
                edgeBuckets[y2 - minY] = edgePool.size() - 1;
            }
        }
        ringStart = ringEnd;
    }

    int activeHead = -1;
    auto insertActiveSorted = [&](int eIdx)
    {
        if (activeHead == -1 || edgePool[eIdx].xVal < edgePool[activeHead].xVal)
        {
            edgePool[eIdx].nextActive = activeHead;
            activeHead = eIdx;
        }
        else
        {
            int s = activeHead;
            while (edgePool[s].nextActive != -1 && edgePool[edgePool[s].nextActive].xVal < edgePool[eIdx].xVal)
                s = edgePool[s].nextActive;
            edgePool[eIdx].nextActive = edgePool[s].nextActive;
            edgePool[s].nextActive = eIdx;
        }
    };
    auto fixInversions = [&]()
    {
        int* pCurrIdx = &activeHead;
        while (edgePool[*pCurrIdx].nextActive != -1)
        {
            int nxt = edgePool[*pCurrIdx].nextActive;
            if (edgePool[nxt].xVal < edgePool[*pCurrIdx].xVal)
            {
                edgePool[*pCurrIdx].nextActive = edgePool[nxt].nextActive;
                edgePool[nxt].nextActive = *pCurrIdx;
                *pCurrIdx = nxt;
            }
            pCurrIdx = &(edgePool[*pCurrIdx].nextActive);
        }
    };
    int startY = std::max(minY, -yOffset);
    int endY = std::min(maxY, (int)tileHeight - 1 - yOffset);
    bool inverted = false;

    if (startY > minY)
    {
        for (int y = minY; y < startY; y++)
        {
            int eIdx = edgeBuckets[y - minY];
            while (eIdx != -1)
            {
                int nextIdx = edgePool[eIdx].nextInBucket;
                edgePool[eIdx].xVal += edgePool[eIdx].slope * (startY - y);
                insertActiveSorted(eIdx);
                eIdx = nextIdx;
            }
        }
        int* pCurrIdx = &activeHead;
        while (*pCurrIdx != -1)
        {
            if (edgePool[*pCurrIdx].yMax <= startY)
                *pCurrIdx = edgePool[*pCurrIdx].nextActive;
            else
                pCurrIdx = &(edgePool[*pCurrIdx].nextActive);
        }
    }

    for (int y = startY; y <= endY; y++)
    {
        int* pCurrIdx = &activeHead;
        while (*pCurrIdx != -1)
        {
            if (edgePool[*pCurrIdx].yMax <= y)
                *pCurrIdx = edgePool[*pCurrIdx].nextActive;
            else
                pCurrIdx = &(edgePool[*pCurrIdx].nextActive);
        }
        int eIdx = edgeBuckets[y - minY];
        while (eIdx != -1)
        {
            int nextIdx = edgePool[eIdx].nextInBucket;
            insertActiveSorted(eIdx);
            eIdx = nextIdx;
        }
        if (activeHead == -1)
        {
            inverted = false;
            continue;
        }
        if (inverted)
        {
            fixInversions();
            inverted = false;
        }

        int yy = y + yOffset;
        int left = activeHead;
        while (left != -1 && edgePool[left].nextActive != -1)
        {
            int right = edgePool[left].nextActive;
            int xStart = (edgePool[left].xVal >> 16) + xOffset;
            int xEnd = (edgePool[right].xVal >> 16) + xOffset;
            if (xStart < 0)
                xStart = 0;
            if (xEnd > (int)tileWidth)
                xEnd = (int)tileWidth;
            if (xEnd > xStart)
            {
                if (buf && yy >= 0 && yy < (int)tileHeight)
                {
                    uint16_t* row = buf + (uint32_t)yy * stride + xStart;
                    int len = xEnd - xStart;
                    uint16_t* end = row + len;
                    while (row < end)
                        *row++ = rawColor;
                }
                else
                    map.drawFastHLine(xStart, yy, xEnd - xStart, color);
            }
            left = edgePool[right].nextActive;
        }
        int a = activeHead;
        edgePool[a].xVal += edgePool[a].slope;
        int prevX = edgePool[a].xVal;
        while (edgePool[a].nextActive != -1)
        {
            a = edgePool[a].nextActive;
            edgePool[a].xVal += edgePool[a].slope;
            if (edgePool[a].xVal < prevX)
                inverted = true;
            prevX = edgePool[a].xVal;
        }
    }
}

/**
 * @brief Projects geographic coordinates (Latitude/Longitude) to local pixel coordinates.
 *
 * @details Performs a Web Mercator projection to convert WGS84 decimal degrees into pixel
 *          offsets relative to the top-left tile of a viewport.
 *
 * @param lat Latitude in decimal degrees.
 * @param lon Longitude in decimal degrees.
 * @param px  Output horizontal pixel position relative to the viewport.
 * @param py  Output vertical pixel position relative to the viewport.
 */
void Maps::latLonToPixel(float lat, float lon, int16_t& px, int16_t& py)
{
    const float n = static_cast<float>(1u << vectorZoom);
    const float tx = (lon + 180.0f) / 360.0f * n;
    const float merc_n = calcMercatorN(lat);
    const float ty = (1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * n;
    px = static_cast<int16_t>((tx - mapTlX) * 256.0f);
    py = static_cast<int16_t>((ty - mapTlY) * 256.0f);
}

/**
 * @brief Draws a thick line using parallel Bresenham lines offset perpendicular to the segment.
 *
 * @details Faster than drawWideLine (no floating-point per segment) at the cost of
 *          no anti-aliasing and minor gap artefacts at sharp corners.
 *          Offsets in Y for near-horizontal segments, in X for near-vertical ones.
 *
 * @param map   Target sprite.
 * @param x0    Start X.
 * @param y0    Start Y.
 * @param x1    End X.
 * @param y1    End Y.
 * @param width Line width in pixels.
 * @param color RGB565 color.
 */
void Maps::drawThickLine(MapCanvas& map, int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1, uint8_t width, uint16_t color)
{
    if (width <= 1)
    {
        map.drawLine(x0, y0, x1, y1, color);
        return;
    }
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int8_t half = (int8_t)(width / 2);
    if (abs(dx) >= abs(dy))
    {
        for (int8_t i = -half; i <= half; i++)
            map.drawLine(x0, y0 + i, x1, y1 + i, color);
    }
    else
    {
        for (int8_t i = -half; i <= half; i++)
            map.drawLine(x0 + i, y0, x1 + i, y1, color);
    }
}

/**
 * @brief Rasterizes a clipped line directly into the sprite framebuffer.
 *
 * @details Bresenham line clipped to the sprite bounds with a parametric
 *          (Liang-Barsky) clip, writing RGB565 pixels straight into PSRAM the
 *          same way the polygon scanline does, avoiding the per-segment LGFX
 *          draw call overhead.
 *
 * @param buf      Framebuffer pointer (uint16_t elements).
 * @param stride   Framebuffer stride in uint16_t elements.
 * @param x0       Segment start X.
 * @param y0       Segment start Y.
 * @param x1       Segment end X.
 * @param y1       Segment end Y.
 * @param rawColor Color as stored in the framebuffer (byte-swapped RGB565).
 * @param w        Sprite width.
 * @param h        Sprite height.
 */
static void drawLineRaw(uint16_t* buf, uint32_t stride, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1, uint16_t rawColor, int16_t w, int16_t h)
{
    // Fast path: line fully inside tile — skip clipping entirely
    if (x0 >= 0 && x0 < w && x1 >= 0 && x1 < w &&
        y0 >= 0 && y0 < h && y1 >= 0 && y1 < h)
    {
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int dx = abs(x1 - x0);
        int dy = -abs(y1 - y0);
        int err = dx + dy;
        while (true)
        {
            buf[(uint32_t)y0 * stride + x0] = rawColor;
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
        return;
    }

    // Cohen-Sutherland clipping with integer-only math (no float)
    int dx = x1 - x0;
    int dy = y1 - y0;
    int u1 = 0;
    int u2 = 1 << 16;
    int p0 = -dx, p1 = dx, p2 = -dy, p3 = dy;
    int q0 = x0, q1 = w - 1 - x0, q2 = y0, q3 = h - 1 - y0;

    for (int i = 0; i < 4; i++)
    {
        int pi, qi;
        switch (i)
        {
            case 0: pi = p0; qi = q0; break;
            case 1: pi = p1; qi = q1; break;
            case 2: pi = p2; qi = q2; break;
            default: pi = p3; qi = q3; break;
        }
        if (pi == 0)
        {
            if (qi < 0)
                return;
        }
        else
        {
            int32_t r = ((int32_t)qi << 16) / pi;
            if (pi < 0)
            {
                if (r > u2)
                    return;
                if (r > u1)
                    u1 = r;
            }
            else
            {
                if (r < u1)
                    return;
                if (r < u2)
                    u2 = r;
            }
        }
    }

    int xs = x0 + (int)(((int32_t)dx * u1 + 32768) >> 16);
    int ys = y0 + (int)(((int32_t)dy * u1 + 32768) >> 16);
    int xe = x0 + (int)(((int32_t)dx * u2 + 32768) >> 16);
    int ye = y0 + (int)(((int32_t)dy * u2 + 32768) >> 16);

    int sx = (xs < xe) ? 1 : -1;
    int sy = (ys < ye) ? 1 : -1;
    dx = abs(xe - xs);
    dy = -abs(ye - ys);
    int err = dx + dy;
    while (true)
    {
        buf[(uint32_t)ys * stride + xs] = rawColor;
        if (xs == xe && ys == ye)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            xs += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            ys += sy;
        }
    }
}

/**
 * @brief Draws a thick line directly into the framebuffer.
 *
 * @details Same offset-parallels approach as drawThickLine(), but rasterized
 *          straight into PSRAM via drawLineRaw() instead of the LGFX API.
 *
 * @param buf      Framebuffer pointer (uint16_t elements).
 * @param stride   Framebuffer stride in uint16_t elements.
 * @param x0       Segment start X.
 * @param y0       Segment start Y.
 * @param x1       Segment end X.
 * @param y1       Segment end Y.
 * @param width    Line width in pixels.
 * @param rawColor Color as stored in the framebuffer (byte-swapped RGB565).
 * @param w        Sprite width.
 * @param h        Sprite height.
 */
static void drawThickLineRaw(uint16_t* buf, uint32_t stride, int16_t x0, int16_t y0,
                             int16_t x1, int16_t y1, uint8_t width, uint16_t rawColor,
                             int16_t w, int16_t h)
{
    if (width <= 1)
    {
        drawLineRaw(buf, stride, x0, y0, x1, y1, rawColor, w, h);
        return;
    }
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int8_t half = (int8_t)(width / 2);

    // Axis-aligned fast path: the parallel passes cover an exact rectangle,
    // so fill it with one contiguous write per row instead of N Bresenhams.
    if (dx == 0 || dy == 0)
    {
        int16_t xa;
        int16_t xb;
        int16_t ya;
        int16_t yb;
        if (dx == 0)
        {
            xa = x0 - half;
            xb = x0 + half;
            ya = (y0 < y1) ? y0 : y1;
            yb = (y0 < y1) ? y1 : y0;
        }
        else
        {
            ya = y0 - half;
            yb = y0 + half;
            xa = (x0 < x1) ? x0 : x1;
            xb = (x0 < x1) ? x1 : x0;
        }
        int16_t cx0 = (xa > 0) ? xa : 0;
        int16_t cx1 = (xb < w - 1) ? xb : (w - 1);
        int16_t cy0 = (ya > 0) ? ya : 0;
        int16_t cy1 = (yb < h - 1) ? yb : (h - 1);
        if (cx0 > cx1 || cy0 > cy1)
            return;
        for (int16_t yy = cy0; yy <= cy1; yy++)
        {
            uint16_t* p = buf + (uint32_t)yy * stride + cx0;
            for (int16_t xx = cx0; xx <= cx1; xx++)
                *p++ = rawColor;
        }
        return;
    }

    if (abs(dx) >= abs(dy))
    {
        for (int8_t i = -half; i <= half; i++)
            drawLineRaw(buf, stride, x0, y0 + i, x1, y1 + i, rawColor, w, h);
    }
    else
    {
        for (int8_t i = -half; i <= half; i++)
            drawLineRaw(buf, stride, x0 + i, y0, x1 + i, y1, rawColor, w, h);
    }
}

/**
 * @brief Renders a vector line (roads, paths, etc.) onto a sprite.
 *
 * @details This function decodes compressed vector data and draws it as a series of
 *          connected segments. It supports "casing" (drawing a slightly wider, darker
 *          background line to create an outline effect) and applies dynamic Level of
 *          Detail (LOD) filtering based on the current zoom level to optimize performance.
 *
 * @param ref Reference to the feature data, including coordinates and style.
 * @param map The target MapCanvas for rendering.
 * @param isCasing  If true, renders the line outline (wider and darkened).
 *                  If false, renders the main line body.
 */
void Maps::renderVectorLine(const FeatureRef& ref, MapCanvas& map, bool isCasing)
{
    if (ref.coordCount < 2)
        return;

    if (isCasing)
    {
        if (ref.priority < 13)
            return;
    }
    
    if (ref.coordCount * 2 > decodedCoords.capacity())
        return;
    int16_t* coords = decodedCoords.data();
    uint8_t* p = ref.ptr;
    int32_t curX = 0;
    int32_t curY = 0;
    int16_t tOffX = ref.tileOffsetX;
    int16_t tOffY = ref.tileOffsetY;
    
    for (uint16_t i = 0; i < ref.coordCount; i++)
    {
        curX += NavReader::decodeZigZag(NavReader::readVarInt(p));
        curY += NavReader::decodeZigZag(NavReader::readVarInt(p));
        coords[i * 2] = tOffX + (curX >> 4);
        coords[i * 2 + 1] = tOffY + (curY >> 4);
    }

    uint16_t color;
    if (isCasing)
        color = darkenRGB565(ref.color, 0.3f);
    else
        color = ref.color;

    float widthF = (ref.width == 0 ? 2 : ref.width) / 2.0f;
    if (isCasing)
        widthF += 1.0f;

    int16_t lastPx = -32768;
    int16_t lastPy = -32768;
    int16_t w = (int16_t)tileWidth;
    int16_t h = (int16_t)tileHeight;
    const int16_t lodThreshold = getLODThreshold(vectorZoom);

    uint16_t* buf = static_cast<uint16_t*>(map.getBuffer());
    uint32_t stride = buf ? (map.bufferLength() / (tileHeight * 2)) : 0;
    uint16_t rawColor = (color >> 8) | (color << 8);

    for (uint16_t i = 0; i < ref.coordCount; i++)
    {
        int16_t px = coords[i * 2];
        int16_t py = coords[i * 2 + 1];

        if (i > 0)
        {
            if (abs(px - lastPx) < lodThreshold && abs(py - lastPy) < lodThreshold)
            {
                if (i < ref.coordCount - 1)
                    continue;
            }

            if (!((px < 0 && lastPx < 0) || (px >= w && lastPx >= w) || (py < 0 && lastPy < 0) || (py >= h && lastPy >= h)))
            {
                uint8_t iWidth = (widthF <= 1.1f) ? 1 : (uint8_t)(widthF + 0.5f);
                if (buf)
                    drawThickLineRaw(buf, stride, lastPx, lastPy, px, py, iWidth, rawColor, w, h);
                else
                    drawThickLine(map, lastPx, lastPy, px, py, iWidth, color);
            }
        }
        lastPx = px;
        lastPy = py;
    }
}

/**
 * @brief Renders a vector polygon (parks, water, buildings) onto a sprite.
 * 
 * @details This function processes encoded vector data to reconstruct polygon geometry, 
 *          including support for multiple rings (holes or multi-part polygons). It includes 
 *          coordinate simplification for performance and optional outline (casing) rendering 
 *          at high zoom levels.
 *
 * @param ref  Reference to the feature data, including vertex pointers, 
 *             colors, and styling metadata.
 * @param map  The target MapCanvas where the polygon and its outline will be drawn.
 */
void Maps::renderVectorPolygon(const FeatureRef& ref, MapCanvas& map)
{
    if (ref.coordCount < 3 || ref.coordCount > MAX_POLYGON_POINTS)
        return;

    if (ref.coordCount * 2 > decodedCoords.capacity())
        return;
    int16_t* coords = decodedCoords.data();
    uint8_t* p = ref.ptr;
    int32_t curX = 0;
    int32_t curY = 0;
    
    for (uint16_t i = 0; i < ref.coordCount; i++)
    {
        curX += NavReader::decodeZigZag(NavReader::readVarInt(p));
        curY += NavReader::decodeZigZag(NavReader::readVarInt(p));
        coords[i * 2] = ref.tileOffsetX + (curX >> 4);
        coords[i * 2 + 1] = ref.tileOffsetY + (curY >> 4);
    }

    uint8_t* p_rings = p;
    uint16_t ringCount = 0;
    const uint16_t* ringEndsPtr = nullptr;
    size_t ringOffset = (size_t)(p_rings - ref.ptr);
    if (ringOffset + 2 <= ref.payloadSize)
    {
        size_t ringBytesAvail = ref.payloadSize - ringOffset;
        ringCount = p_rings[0] | (p_rings[1] << 8);
        if (ringCount > 0 && ringCount <= ringEndsCache.capacity() &&
            (size_t)(2 + ringCount * 2) <= ringBytesAvail)
        {
            uint8_t* p_curr_ring = p_rings + 2;
            uint16_t* dst = ringEndsCache.data();
            uint16_t prevEnd = 0;
            bool valid = true;
            for (int r = 0; r < (int)ringCount; r++)
            {
                uint16_t ringEnd = p_curr_ring[0] | (p_curr_ring[1] << 8);
                if (ringEnd <= prevEnd || ringEnd > ref.coordCount)
                {
                    valid = false;
                    break;
                }
                dst[r] = ringEnd;
                prevEnd = ringEnd;
                p_curr_ring += 2;
            }
            if (valid)
                ringEndsPtr = dst;
            else
                ringCount = 0;
        }
        else
            ringCount = 0;
    }

    if (ref.coordCount > projBuf32X.capacity())
        return;
    int minPx = INT_MAX;
    int maxPx = INT_MIN;
    int minPy = INT_MAX;
    int maxPy = INT_MIN;
    int16_t lastX = -32768;
    int16_t lastY = -32768;
    uint16_t actualPoints = 0;
    const int16_t lodThreshold = getLODThreshold(vectorZoom);

    for (size_t i = 0; i < ref.coordCount; i++)
    {
        int16_t curX = coords[i * 2];
        int16_t curY = coords[i * 2 + 1];
        if (ringCount == 0 && i > 0 && abs(curX - lastX) < lodThreshold && abs(curY - lastY) < lodThreshold && i < ref.coordCount - 1)
            continue;
        projBuf32X[actualPoints] = curX;
        projBuf32Y[actualPoints] = curY;
        if (curX < minPx)
            minPx = curX;
        if (curX > maxPx)
            maxPx = curX;
        if (curY < minPy)
            minPy = curY;
        if (curY > maxPy)
            maxPy = curY;
        lastX = curX;
        lastY = curY;
        actualPoints++;
    }
    if (maxPx < 0 || minPx >= (int)tileWidth || maxPy < 0 || minPy >= (int)tileHeight)
        return;

    int* px = projBuf32X.data();
    int* py = projBuf32Y.data();
    fillPolygonGeneral(map, px, py, actualPoints, ref.color, 0, 0, ringCount, ringEndsPtr);
    if (ref.casing && vectorZoom >= 16)
    {
        uint16_t outlineColor = darkenRGB565(ref.color, 0.35f);
        uint16_t rawOutline = (outlineColor >> 8) | (outlineColor << 8);
        uint16_t* buf = static_cast<uint16_t*>(map.getBuffer());
        uint32_t stride = buf ? (map.bufferLength() / (tileHeight * 2)) : 0;
        const int16_t w = (int16_t)tileWidth;
        const int16_t h = (int16_t)tileHeight;
        int ringStart = 0;
        uint16_t numRings;
        if (ringCount > 0)
            numRings = ringCount;
        else
            numRings = 1;

        for (uint16_t r = 0; r < numRings; r++)
        {
            uint16_t ringEnd;
            if (ringEndsPtr && r < ringCount)
                ringEnd = ringEndsPtr[r];
            else
                ringEnd = actualPoints;

            if (ringEnd > actualPoints)
                ringEnd = actualPoints;

            for (uint16_t j = ringStart; j < ringEnd; j++)
            {
                uint16_t next;
                if (j + 1 < ringEnd)
                    next = j + 1;
                else
                    next = ringStart;

                if (buf)
                    drawLineRaw(buf, stride, px[j], py[j], px[next], py[next], rawOutline, w, h);
                else
                    map.drawLine(px[j], py[j], px[next], py[next], outlineColor);
            }
            ringStart = ringEnd;
        }
    }
}

/**
 * @brief Renders a vector point (POI) as a filled circle.
 * 
 * @details Decodes the point's coordinates using ZigZag/VarInt, applies the tile offset, 
 *          and draws a circle at the resulting position if it falls within the tile bounds.
 * 
 * @param ref Reference to the point feature data and styling.
 * @param map The target sprite for rendering.
 */
void Maps::renderVectorPoint(const FeatureRef& ref, MapCanvas& map)
{
    if (ref.coordCount == 0)
        return;
    uint8_t* p = ref.ptr;
    int32_t x = NavReader::decodeZigZag(NavReader::readVarInt(p));
    int32_t y = NavReader::decodeZigZag(NavReader::readVarInt(p));
    int16_t px = ref.tileOffsetX + (x >> 4);
    int16_t py = ref.tileOffsetY + (y >> 4);

    if (px < 0 || px >= (int)tileWidth || py < 0 || py >= (int)tileHeight)
        return;

    uint16_t* buf = static_cast<uint16_t*>(map.getBuffer());
    if (!buf)
    {
        map.fillCircle(px, py, 3, ref.color);
        return;
    }
    uint32_t stride = map.bufferLength() / (tileHeight * 2);
    uint16_t rawColor = (ref.color >> 8) | (ref.color << 8);
    // Filled circle (radius 3) written directly to PSRAM.
    for (int dy = -3; dy <= 3; dy++)
    {
        int rowY = py + dy;
        if (rowY < 0 || rowY >= (int)tileHeight)
            continue;
        int halfW = (int)sqrtf((float)(9 - dy * dy));
        int x0 = px - halfW;
        int x1 = px + halfW;
        if (x0 < 0)
            x0 = 0;
        if (x1 >= (int)tileWidth)
            x1 = (int)tileWidth - 1;
        uint16_t* row = buf + (uint32_t)rowY * stride + x0;
        for (int x = x0; x <= x1; x++)
            *row++ = rawColor;
    }
}

/**
 * @brief Renders vector-map text labels with collision detection.
 * 
 * @details Decodes label coordinates and text content from the feature payload, then
 *          checks for overlaps against previously placed labels using a padding-aware 
 *          AABB (Axis-Aligned Bounding Box) test. If no collision is found, the text 
 *          is drawn and its bounds are added to the placedLabels list.
 * 
 * @param ref Reference to the text feature data (coords, length, string).
 * @param map The target sprite for rendering.
 * @param placedLabels  Vector tracking occupied screen areas to prevent overlapping text.
 */
void Maps::renderVectorText(const FeatureRef& ref, MapCanvas& map, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels)
{
    uint8_t* p = ref.ptr;
    int16_t tx;
    int16_t ty;
    memcpy(&tx, p, 2);
    memcpy(&ty, p + 2, 2);
    int16_t px = ref.tileOffsetX + (tx >> 4);
    int16_t py = ref.tileOffsetY + (ty >> 4);
    uint8_t textLen = p[4];
    if (textLen == 0 || textLen >= 128)
        return;
    char textBuf[128];
    memcpy(textBuf, p + 5, textLen);
    textBuf[textLen] = '\0';

    // Fast-reject for labels clearly outside the viewport
    if (px < -100 || px > (int)tileWidth + 100 || py < -50 || py > (int)tileHeight + 50)
        return;

    // Scales adjusted for sharpness: base size 1.0 prevents VLW distortion
    float scale = (ref.width == 0) ? 1.0f : (ref.width == 1) ? 1.2f : 1.5f;
    map.setTextSize(scale);

    // Fast heuristic pre-check (Assume average char width ~8-10px scaled)
    int estimatedWidth = textLen * (8 * scale);
    int th = map.fontHeight();
    int elx = px - estimatedWidth / 2;
    int ely = py - th;
    const int PAD = 4;

    // Fast heuristic check: estimatedWidth >= real width, so a collision here is definitive.
    for (const auto& r : placedLabels)
    {
        if (elx - PAD < r.x + r.w && elx + estimatedWidth + PAD > r.x &&
            ely - PAD < r.y + r.h && ely + th + PAD > r.y)
            return;
    }

    int tw = map.textWidth(textBuf);
    int lx = px - tw / 2;
    int ly = py - th;

    if (lx + tw < 0 || lx >= (int)tileWidth || ly + th < 0 || ly >= (int)tileHeight)
        return;

    // Precise check with real width, skipping rows clearly out of range.
    for (const auto& r : placedLabels)
    {
        if (abs(ly - r.y) > th + PAD * 2)
            continue;
        if (lx - PAD < r.x + r.w && lx + tw + PAD > r.x && ly - PAD < r.y + r.h && ly + th + PAD > r.y)
            return;
    }

    map.setTextColor(ref.color);
    map.setTextDatum(lgfx::top_center);
    map.drawString(textBuf, px, ly);
    map.setTextDatum(lgfx::top_left);

    if (placedLabels.size() < placedLabels.capacity())
    {
        placedLabels.push_back({(int16_t)lx, (int16_t)ly, (int16_t)tw, (int16_t)th});
    }
}

/**
 * @brief Enqueues all tiles of the viewport grid into pendingTiles.
 *
 * @details For 3x3 grids uses a spiral order (corners first, centre last) so the
 *          LIFO consumer (mapRenderTask) renders the centre tile first.
 *          For 4x4 grids uses row-major order.
 *
 * @param centerTileIdxX Global X index of the centre tile.
 * @param centerTileIdxY Global Y index of the centre tile.
 * @param type           Tile type to enqueue (TILE_NAV or TILE_PNG).
 */
void Maps::enqueueTileGrid(uint32_t centerTileIdxX, uint32_t centerTileIdxY, TileType type)
{
    const int8_t gridOffset = tilesGrid / 2;
    if (tilesGrid == 3)
    {
        for (int i = 0; i < 9; i++)
        {
            int dx = NAV_SPIRAL_ORDER[i][0];
            int dy = NAV_SPIRAL_ORDER[i][1];
            pendingTiles.push_back({(uint32_t)(centerTileIdxX - gridOffset + dx),
                                    (uint32_t)(centerTileIdxY - gridOffset + dy),
                                    (int16_t)(dx * 256), (int16_t)(dy * 256), type});
        }
    }
    else
    {
        for (int dy = 0; dy < tilesGrid; dy++)
            for (int dx = 0; dx < tilesGrid; dx++)
                pendingTiles.push_back({(uint32_t)(centerTileIdxX - gridOffset + dx),
                                        (uint32_t)(centerTileIdxY - gridOffset + dy),
                                        (int16_t)(dx * 256), (int16_t)(dy * 256), type});
    }
    pendingTilesNotEmpty = true;
}

/**
 * @brief Initializes and queues the vector viewport for rendering.
 *
 * @details Updates the viewport anchor and queues the complete vector tile grid. The map argument
 *          remains in the signature for API compatibility and is not used directly here.
 *
 * @param centerLat Latitude of the viewport center.
 * @param centerLon Longitude of the viewport center.
 * @param zoom Target zoom level.
 * @param map Map sprite kept for API compatibility.
 * @return true after the viewport state has been prepared.
 */
bool Maps::renderVectorViewport(float centerLat, float centerLon, uint8_t zoom, MapCanvas& map)
{
    const uint32_t centerTileIdxX = lon2tilex(centerLon, zoom);
    const uint32_t centerTileIdxY = lat2tiley(centerLat, zoom);
    const int8_t gridOffset = tilesGrid / 2;
    mapTlX = (float)(centerTileIdxX - gridOffset);
    mapTlY = (float)(centerTileIdxY - gridOffset);
    vectorPending = false;
    vectorSteps.clear();
    vectorZoom = zoom;
    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        redrawMap = true;
        pendingTiles.clear();
        pendingTilesNotEmpty = false;
        enqueueTileGrid(centerTileIdxX, centerTileIdxY, TILE_NAV);
        xSemaphoreGiveRecursive(mapMutex);
    }
    return true;
}

/**
 * @brief Looks up a vector tile in the LRU cache, loading it from storage on miss.
 *
 * @details On a cache hit, updates lastAccess and pins the entry. On a miss, opens
 *          the zoom-level pack, reads the tile into PSRAM, evicts the LRU unpinned
 *          entry if the cache is full, and inserts the new entry.
 *
 * @param tileX      Global X tile index.
 * @param tileY      Global Y tile index.
 * @param zoom       Current zoom level.
 * @param outDataSize Output: byte size of the returned buffer.
 * @return Pointer to tile data in PSRAM, or nullptr on failure.
 */
uint8_t* Maps::vectorCacheLookupOrLoad(uint32_t tileX, uint32_t tileY, uint8_t zoom, size_t& outDataSize)
{
    uint32_t tileHash = (uint32_t(zoom) << 28) | (uint32_t(tileX & 0x3FFF) << 14) | uint32_t(tileY & 0x3FFF);
    int cacheIdx = -1;
    for (int i = 0; i < (int)vectorCache.size(); i++)
    {
        if (vectorCache[i].tileHash == tileHash)
        {
            cacheIdx = i;
            break;
        }
    }
    if (cacheIdx >= 0)
    {
        vectorCache[cacheIdx].lastAccess = ++cacheCounter;
        vectorCache[cacheIdx].isPinned = true;
        outDataSize = vectorCache[cacheIdx].size;
        return vectorCache[cacheIdx].data;
    }

    if (!NavReader::openPack(zoom))
        return nullptr;

    uint32_t offset;
    uint32_t size;

    if (!NavReader::findTileInPack(tileX, tileY, offset, size))
        return nullptr;

    uint8_t* data = static_cast<uint8_t*>(heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM));
    if (!data)
    {
        for (int i = (int)vectorCache.size() - 1; i >= 0; i--)
        {
            if (!vectorCache[i].isPinned)
            {
                heap_caps_free(vectorCache[i].data);
                vectorCache.erase(vectorCache.begin() + i);
            }
        }
        data = static_cast<uint8_t*>(heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM));
        if (!data)
            return nullptr;
    }

    if (storage.seekAndRead(NavReader::packFile, offset, data, size) != size)
    {
        heap_caps_free(data);
        return nullptr;
    }

    if (vectorCache.size() >= NAV_DATA_CACHE_SIZE)
    {
        int lru = -1;
        for (int i = 0; i < (int)vectorCache.size(); i++)
        {
            if (!vectorCache[i].isPinned && (lru == -1 || vectorCache[i].lastAccess < vectorCache[lru].lastAccess))
                lru = i;
        }
        if (lru != -1)
        {
            heap_caps_free(vectorCache[lru].data);
            vectorCache.erase(vectorCache.begin() + lru);
        }
    }

    vectorCache.push_back({data, size, tileHash, ++cacheCounter, true, 0});
    outDataSize = size;
    return data;
}

/**
 * @brief Decodes all vector features from a tile buffer into featurePool and layers.
 *
 * @details Iterates over each feature header, applies zoom LOD and frustum culling,
 *          then appends accepted features to featurePool and their index to the
 *          corresponding priority layer.
 *
 * @param data     Pointer to the raw tile data in PSRAM.
 * @param dataSize Byte size of the tile data.
 * @param screenX  Horizontal pixel offset of this tile in the sprite.
 * @param screenY  Vertical pixel offset of this tile in the sprite.
 * @param zoom     Current zoom level.
 */
void Maps::decodeVectorFeatures(const uint8_t* data, size_t dataSize, int16_t screenX, int16_t screenY, uint8_t zoom)
{
    uint16_t feature_count;
    memcpy(&feature_count, data + NAV_TILE_HDR_FEAT_COUNT_OFF, 2);
    const uint8_t* p = data + NAV_TILE_HDR_SIZE;
    const uint8_t* end = data + dataSize;
    for (uint16_t i = 0; i < feature_count; i++)
    {
        if (p + NAV_FEAT_HDR_FIXED_SIZE > end)
            break;
        uint8_t geomType = p[NAV_FEAT_GEOM_OFF];
        uint8_t colorIdx = p[NAV_FEAT_COLOR_IDX_OFF];
        uint8_t zp = p[NAV_FEAT_ZP_OFF];
        uint8_t wp = p[NAV_FEAT_WP_OFF];
        uint8_t bx1 = p[NAV_FEAT_BX1_OFF];
        uint8_t by1 = p[NAV_FEAT_BY1_OFF];
        uint8_t bx2 = p[NAV_FEAT_BX2_OFF];
        uint8_t by2 = p[NAV_FEAT_BY2_OFF];

        const uint8_t* hp = p + NAV_FEAT_HDR_FIXED_SIZE;
        uint16_t cc = (uint16_t)NavReader::readVarIntU(hp);
        uint16_t ps = (uint16_t)NavReader::readVarIntU(hp);
        const uint8_t* payload = hp;
        if (payload + ps > end)
            break;
        uint16_t colorRgb565 = NavReader::paletteColor(colorIdx);

        if ((zp >> 4) <= zoom)
        {
            if (screenX + bx2 < 0 || screenX + bx1 > (int)tileWidth || screenY + by2 < 0 || screenY + by1 > (int)tileHeight)
            {
                p = payload + ps;
                continue;
            }
            int16_t dimX = bx2 - bx1;
            int16_t dimY = by2 - by1;
            uint8_t minDim = (zoom >= 9 && zoom <= 11) ? 3 : 1;
            if ((geomType == (uint8_t)NavGeomType::Polygon || geomType == (uint8_t)NavGeomType::LineString) && dimX < minDim && dimY < minDim)
            {
                p = payload + ps;
                continue;
            }

            bool hasCasingHdr = (wp & 0x80) != 0;
            if (geomType == (uint8_t)NavGeomType::Polygon && !hasCasingHdr)
            {
                uint32_t areaCullThreshold = getPolygonAreaCullThreshold(zoom);
                if (areaCullThreshold > 0 && (uint32_t)dimX * (uint32_t)dimY < areaCullThreshold)
                {
                    p = payload + ps;
                    continue;
                }
            }
            if (featurePool.size() < MAX_FEATURE_POOL_SIZE)
            {
                uint16_t poolIdx = (uint16_t)featurePool.size();
                bool hasCasing = hasCasingHdr;
                featurePool.push_back({(uint8_t*)payload, (NavGeomType)geomType, ps, cc, screenX, screenY, colorRgb565, (uint8_t)(wp & 0x7F), hasCasing, bx1, by1, bx2, by2, (uint8_t)(zp & 0x0F)});
                uint8_t priority = zp & 0x0F;
                if (priority < 16)
                {
                    if (geomType == (uint8_t)NavGeomType::Text)
                        layersText[priority].push_back(poolIdx);
                    else
                        layers[priority].push_back(poolIdx);
                    if (geomType == (uint8_t)NavGeomType::LineString && hasCasing)
                        layersCasing[priority].push_back(poolIdx);
                }
            }
        }
        p = payload + ps;
    }
}

/**
 * @brief Fetches and decodes a single vector tile from cache or storage.
 *
 * @param tileX   The global X index of the tile.
 * @param tileY   The global Y index of the tile.
 * @param zoom    The current map zoom level.
 * @param screenX The horizontal pixel offset on the target sprite.
 * @param screenY The vertical pixel offset on the target sprite.
 * @param map     The target sprite (unused, kept for API compatibility).
 */
void Maps::renderVectorTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, MapCanvas &map)
{
    size_t dataSize = 0;
    uint8_t* data = vectorCacheLookupOrLoad(tileX, tileY, zoom, dataSize);
    if (!data || dataSize < NAV_TILE_HDR_SIZE)
        return;
    decodeVectorFeatures(data, dataSize, screenX, screenY, zoom);
}