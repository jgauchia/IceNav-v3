/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.9
 * @date 2026-06
 */

#include "maps.hpp"
#include <cmath>
#include <climits>
#include "esp_task_wdt.h"
#include "tasks.hpp"
#include "mainScr.hpp"
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
extern TrackVector trackData;
static const char* TAG = "Maps";

/**
 * @brief Map Class constructor
 */
Maps::Maps() : navLastZoom_(0),
               navNeedsRender_(true),
               navTlTileX_(-1),
               navTlTileY_(-1),
               lastRenderedHeading(0xFFFF),
               lastRenderedArrowPos({-32768, -32768}),
               lastRenderedDisplayOffsetX(-32768),
               lastRenderedDisplayOffsetY(-32768),
               _mapTilt(60.0f),
               _focalLength(300.0f){
    static_assert(Maps::MAX_FEATURE_POOL_SIZE <= 65535U,
        "featurePool index stored as uint16_t — pool size must not exceed 65535");
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
    }

    ringEndsCache.reserve(MAX_POLYGON_POINTS);
    placedLabelsCache.reserve(MAX_PLACED_LABELS);
    navDataCache.reserve(NAV_DATA_CACHE_SIZE);
    mapMutex = xSemaphoreCreateRecursiveMutex();
    mapEventGroup = xEventGroupCreate();
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
 * @brief Load No Map image
 */
void Maps::showNoMap(TFT_eSprite &map)
{
    int16_t centerX = (Maps::mapScrWidth / 2) - 50;
    int16_t centerY = (Maps::mapScrHeight / 2) - 50;
    map.drawPngFile(noMapFile, centerX, centerY);
    map.drawCenterString("NO MAP FOUND", (Maps::mapScrWidth / 2), (Maps::mapScrHeight >> 1) + 65, &fonts::DejaVu18);
}


/**
 * @brief Initialize map sprites and variables
 * 
 * @param mapHeight Map height
 * @param mapWidth Map width
 */
void Maps::initMap(uint16_t mapHeight, uint16_t mapWidth)
{
    Maps::mapScrHeight = mapHeight;
    Maps::mapScrWidth = mapWidth;
    Maps::mapTempSprite.createSprite(Maps::tileWidth, Maps::tileHeight);
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
void Maps::drawTrack(TFT_eSprite& map)
{
    for (size_t i = 1; i < trackData.size(); ++i)
    {
        const auto &p1 = trackData[i - 1];
        const auto &p2 = trackData[i];
        int16_t x1;
        int16_t y1;
        int16_t x2;
        int16_t y2;
        latLonToPixel(p1.lat, p1.lon, x1, y1);
        latLonToPixel(p2.lat, p2.lon, x2, y2);
        if ((x1 >= 0 && x1 < tileWidth && y1 >= 0 && y1 < tileHeight) || (x2 >= 0 && x2 < tileWidth && y2 >= 0 && y2 < tileHeight))
            map.drawWideLine(x1, y1, x2, y2, 3, TFT_BLUE);
    }
}

/**
 * @brief Request track redraw
 */
void Maps::redrawTrack()
{
    navNeedsRender_ = true;
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

    const float baseLat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
    const float baseLon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

    if (mapSet.vectorMap)
    {
        const uint32_t centerTileIdxX = lon2tilex(baseLon, zoom);
        const uint32_t centerTileIdxY = lat2tiley(baseLat, zoom);
        const int8_t gridOffset = tilesGrid / 2;
        const int32_t currentTlX = (int32_t)centerTileIdxX - gridOffset;
        const int32_t currentTlY = (int32_t)centerTileIdxY - gridOffset;
        bool zoomChanged = (zoom != navLastZoom_);
        bool tileChanged = (currentTlX != (int32_t)navTlTileX_ || currentTlY != (int32_t)navTlTileY_);

        if (zoomChanged)
            navNeedsRender_ = true;

        if (!zoomChanged && !tileChanged && !navNeedsRender_ && pendingTiles.empty())
            return;

        if (pendingTiles.size() > (tilesGrid * tilesGrid))
            return;

        Maps::isMapFound = renderNavViewport(baseLat, baseLon, zoom, Maps::mapTempSprite);
        navLastZoom_ = zoom;
        navNeedsRender_ = false;
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
        navTlTileX_ = (float)tlX;
        navTlTileY_ = (float)tlY;
        navLastZoom_ = zoom;
        Maps::mapTempSprite.fillSprite(TFT_WHITE);
        Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};
        bool centerFound = false;

        if (tilesGrid == 3)
        {
            for (int i = 0; i < 9; i++)
                loadPngTileIntoSprite(tlX, tlY, PNG_SPIRAL_ORDER[i][0], PNG_SPIRAL_ORDER[i][1],
                                      centerTileIdxX, centerTileIdxY, zoom, centerFound);
        }
        else
        {
            for (int gy = 0; gy < tilesGrid; gy++)
                for (int gx = 0; gx < tilesGrid; gx++)
                    loadPngTileIntoSprite(tlX, tlY, gx, gy,
                                          centerTileIdxX, centerTileIdxY, zoom, centerFound);
        }

        Maps::isMapFound = centerFound;
        if (!centerFound)
            showNoMap(mapTempSprite);

        if (Maps::isMapFound && Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
            Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &wptPosX, &wptPosY);
        else
        {
            Maps::wptPosX = -1;
            Maps::wptPosY = -1;
        }

        if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            drawTrack(mapTempSprite);
            redrawMap = true;
            xSemaphoreGiveRecursive(mapMutex);
        }
        xEventGroupSetBits(mapEventGroup, MAP_EVENT_DONE);
    }
}

/**
 * @brief Background task for map rendering
 */
void Maps::mapRenderTask(void* pvParameters)
{
    Maps* instance = static_cast<Maps*>(pvParameters);
    uint8_t lastZoom = 0;

    while (1)
    {
        if (!instance->pendingTiles.empty())
        {
            if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
            {
                if (instance->mapTempSprite.getBuffer() == nullptr)
                {
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
                bool zoomChanged = (instance->zoomLevel != lastZoom);
                bool fullReset = zoomChanged || (instance->pendingTiles.size() >= (tilesGrid * tilesGrid));
                lastZoom = instance->zoomLevel;

                if (fullReset)
                {
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_DONE | MAP_EVENT_ERROR);
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_START);

                    if (zoomChanged)
                    {
                        for (auto& entry : instance->navDataCache)
                            heap_caps_free(entry.data);

                        instance->navDataCache.clear();
                    }

                    instance->featurePool.clear();
                    instance->decodedCoords.clear();
                    for (int i = 0; i < 16; i++)
                    {
                        instance->layers[i].clear();
                        instance->layersCasing[i].clear();
                    }

                    if (mapSet.vectorMap)
                        NavReader::openPack(instance->zoomLevel);
                }

                // Yields mutex briefly so other tasks can run between tile renders.
                // Returns true if rendering should abort (mutex lost or new viewport pending).
                auto yieldTile = [&]() -> bool
                {
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(1);
                    if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                        return true;
                    if (instance->pendingTiles.size() >= (size_t)(tilesGrid * tilesGrid))
                        return true;
                    return false;
                };

                // Yields mutex briefly between feature render passes (endWrite/startWrite around the pause).
                // Returns true if rendering should abort.
                auto yieldFeature = [&]() -> bool
                {
                    instance->mapTempSprite.endWrite();
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    vTaskDelay(pdMS_TO_TICKS(2));
                    if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                        return true;
                    if (!instance->pendingTiles.empty())
                        return true;
                    instance->mapTempSprite.startWrite();
                    return false;
                };

                bool aborted = false;
                while (!instance->pendingTiles.empty())
                {
                    PendingTile t = instance->pendingTiles.back();
                    instance->pendingTiles.pop_back();
                    if (instance->pendingTiles.empty())
                        instance->pendingTilesNotEmpty_ = false;
                    if (t.type == TILE_NAV)
                    {
                        instance->renderNavTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
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
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    continue;
                }

                if (!mapSet.vectorMap)
                {
                    instance->drawTrack(instance->mapTempSprite);
                    instance->redrawMap = true;
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                    xSemaphoreGiveRecursive(instance->mapMutex);
                    continue;
                }

                if (instance->mapTempSprite.getBuffer())
                    instance->mapTempSprite.fillSprite(0xF7BE);

                instance->update3DCache();
                instance->placedLabelsCache.clear();
                instance->mapTempSprite.startWrite();
                uint32_t lastYield = millis();
                uint32_t loopCounter = 0;

                for (int i = 0; i < 16 && !aborted; i++)
                {
                    const auto& layer = instance->layers[i];
                    if (layer.empty())
                        continue;

                    // Pass 1: Polygons, Points, and LineString Outlines (Casing)
                    for (uint16_t idx : layer)
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis();
                            if (now - lastYield > 40)
                            {
                                if (yieldFeature()) { aborted = true; break; }
                                lastYield = millis();
                            }
                        }

                        const auto& feat = instance->featurePool[idx];
                        if (feat.geomType == NavGeomType::Polygon)
                            instance->renderNavPolygon(feat, instance->mapTempSprite);
                        else if (feat.geomType == NavGeomType::Point)
                            instance->renderNavPoint(feat, instance->mapTempSprite);
                        else if (feat.geomType == NavGeomType::LineString)
                            instance->renderNavLineString(feat, instance->mapTempSprite, feat.casing);
                    }

                    if (aborted)
                        break;

                    // Pass 2: LineString bodies (from pre-separated casing list) and Texts
                    for (uint16_t idx : instance->layersCasing[i])
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis();
                            if (now - lastYield > 40)
                            {
                                if (yieldFeature()) { aborted = true; break; }
                                lastYield = millis();
                            }
                        }
                        instance->renderNavLineString(instance->featurePool[idx], instance->mapTempSprite, false);
                    }

                    if (aborted)
                        break;

                    for (uint16_t idx : layer)
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis();
                            if (now - lastYield > 40)
                            {
                                if (yieldFeature()) { aborted = true; break; }
                                lastYield = millis();
                            }
                        }
                        if (instance->featurePool[idx].geomType == NavGeomType::Text)
                            instance->renderNavText(instance->featurePool[idx], instance->mapTempSprite, instance->placedLabelsCache);
                    }

                    esp_task_wdt_reset();
                }

                if (aborted)
                {
                    if (xSemaphoreGetMutexHolder(instance->mapMutex) == xTaskGetCurrentTaskHandle())
                        xSemaphoreGiveRecursive(instance->mapMutex);
                    continue;
                }

                instance->mapTempSprite.endWrite();

                for (auto& entry : instance->navDataCache)
                    entry.isPinned = false;

                instance->displayOffsetX = instance->offsetX;
                instance->displayOffsetY = instance->offsetY;
                instance->lastTileX = instance->tileX;
                instance->lastTileY = instance->tileY;

                instance->drawTrack(instance->mapTempSprite);
                instance->redrawMap = true;

                xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                xSemaphoreGiveRecursive(instance->mapMutex);
                triggerMapRedraw();
            }
        }
        vTaskDelay(1);
    }
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
void Maps::renderPngTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map)
{
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

    uint16_t mapHeading = 0;
    #ifdef ENABLE_COMPASS
    {
        int sensorHeading = 0;
        if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            sensorHeading = globalSensorData.heading;
            xSemaphoreGive(sensorMutex);
        }
        mapHeading = mapSet.mapRotationComp ? (uint16_t)sensorHeading : gps.gpsData.heading;
    }
    #else
        mapHeading = gps.gpsData.heading;
    #endif
    
    Maps::mapTempSprite.pushImage(Maps::wptPosX - 8, Maps::wptPosY - 8, 16, 16, (uint16_t *)waypoint, TFT_BLACK);
    tft.startWrite();

    if (Maps::followGps)
    {
        const float lat = gps.gpsData.latitude;
        const float lon = gps.gpsData.longitude;
        const int8_t gridOffset = tilesGrid / 2;
        Maps::navArrowPosition = Maps::coord2ScreenPos(lon, lat, Maps::zoomLevel, Maps::mapTileSize);

        // Hysteresis: skip redraw if nothing changed
        if (!Maps::redrawMap)
        {
            if (mapHeading == lastRenderedHeading &&
                navArrowPosition.posX == lastRenderedArrowPos.posX &&
                navArrowPosition.posY == lastRenderedArrowPos.posY)
            {
                tft.endWrite();
                xSemaphoreGiveRecursive(mapMutex);
                return;
            }
        }
        lastRenderedHeading = mapHeading;
        lastRenderedArrowPos = navArrowPosition;

        if (_use3DCache)
        {
            // 3D mode: scanline perspective transform with heading baked in
            apply3DPerspective(mapHeading);
        }
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
        // Hysteresis: Skip pushSprite if visual change is minimal and no redraw is forced
        if (!Maps::redrawMap)
        {
            if (displayOffsetX == lastRenderedDisplayOffsetX &&
                displayOffsetY == lastRenderedDisplayOffsetY)
            {
                tft.endWrite();
                xSemaphoreGiveRecursive(mapMutex);
                return;
            }
        }
        lastRenderedDisplayOffsetX = displayOffsetX;
        lastRenderedDisplayOffsetY = displayOffsetY;

        // Manual panning: crop central part of grid adjusted by displayOffsetX/offsetY
        int16_t cropX = (tileWidth - mapScrWidth) / 2 + displayOffsetX;
        int16_t cropY = (tileHeight - mapScrHeight) / 2 + displayOffsetY;
        mapTempSprite.pushSprite(&mapSprite, -cropX, -cropY);
    }

    Maps::redrawMap = false;
    tft.endWrite();
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
    if (trackData.size() > 0)
        return true;
    if (hasWaypoint)
        return true;
    return false;
}

void Maps::update3DCache()
{
    _use3DCache = mapSet.map3D && mapSet.vectorMap && isNavActive() && !_scrolling;
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

    // GPS lands at lower third of the viewport
    const int gpsScreenY = dstH * 3 / 4;

    // Heading rotation: rotate tile-space coords so heading points up
    const float headingRad = static_cast<float>(heading) * (static_cast<float>(M_PI) / 180.0f);
    const float cosH = cosf(headingRad);
    const float sinH = sinf(headingRad);

    // Perspective parameters: horizon at top quarter of screen
    const int horizonScreenY = dstH / 5;
    const float tiltRad = _mapTilt * (static_cast<float>(M_PI) / 180.0f);
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
        float srcRelY = (_focalLength * invScale) * (1.0f - t) / t;

        // sx/sy are linear in x; accumulate in Q16 fixed point so the inner loop
        // drops the per-pixel division, multiplies and float->int conversions.
        const float dsxF = invScale * cosH;
        const float dsyF = invScale * sinH;

        const float sxStart = static_cast<float>(gpsTileX) + (-halfW * invScale) * cosH + srcRelY * sinH;
        const float syStart = static_cast<float>(gpsTileY) - ((halfW * invScale) * sinH + srcRelY * cosH);
        const float sxEnd = sxStart + dsxF * static_cast<float>(dstW);
        const float syEnd = syStart + dsyF * static_cast<float>(dstW);

        // Q16 holds +-32767 integer range; rows near the horizon can exceed it.
        // Valid tile coords stay within a few thousand, so a conservative limit
        // routes any extreme (overflow-prone) row to the float fallback.
        const float Q16_LIMIT = 8000.0f;
        bool fitsQ16 = fabsf(sxStart) < Q16_LIMIT && fabsf(syStart) < Q16_LIMIT &&
                       fabsf(sxEnd) < Q16_LIMIT && fabsf(syEnd) < Q16_LIMIT;

        if (fitsQ16)
        {
            int32_t sxFix = static_cast<int32_t>(sxStart * 65536.0f);
            int32_t syFix = static_cast<int32_t>(syStart * 65536.0f);
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
        else
        {
            float sxF = sxStart;
            float syF = syStart;

            for (int x = 0; x < dstW; x++)
            {
                int sx = (int)sxF;
                int sy = (int)syF;

                if (sx >= 0 && sx < srcW && sy >= 0 && sy < srcH)
                    dstRow[x] = src[sy * srcW + sx];
                else
                    dstRow[x] = skyColor;

                sxF += dsxF;
                syF += dsyF;
            }
        }
    }
}

void Maps::updateMap()
{
    Maps::oldMapTile = {};
    navNeedsRender_ = true;
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
    _scrolling = false;
    Maps::currentMapTile.zoom = Maps::zoomLevel;
    Maps::currentMapTile.tilex = Maps::lon2tilex(lon, Maps::currentMapTile.zoom);
    Maps::currentMapTile.tiley = Maps::lat2tiley(lat, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = lat;
    Maps::currentMapTile.lon = lon;
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
}

/**
 * @brief Smooth scroll the map
 * 
 * @param dx X scroll offset
 * @param dy Y scroll offset
 */
void Maps::scrollMap(int16_t dx, int16_t dy)
{
    pendingDx += dx;
    pendingDy += dy;
    _scrolling = true;

    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return;

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
            Maps::preloadTiles(deltaTileX, deltaTileY);
        else
            updateMap(); // Force vector re-render on tile threshold

        generateMap(zoomLevel);
        Maps::redrawMap = true;
    }

    if (pendingTiles.empty())
    {
        displayOffsetX = offsetX;
        displayOffsetY = offsetY;
        lastTileX = tileX;
        lastTileY = tileY;
    }
    else
    {
        // When rendering is pending (after a swap), we stay at the virtual relative position
        // to avoid jumping until the new grid is complete.
        displayOffsetX = offsetX + (tileX - lastTileX) * mapTileSize;
        displayOffsetY = offsetY + (tileY - lastTileY) * mapTileSize;
    }
    
    xSemaphoreGiveRecursive(mapMutex);
}

 /**
  * @brief Preload PNG tiles
  * 
  * @param dirX X direction 
  * @param dirY Y direction
  */
void Maps::preloadTiles(int8_t dirX, int8_t dirY)
{
    const int16_t tileSize = mapTileSize;
    const int16_t startX = tileX + dirX;
    const int16_t startY = tileY + dirY;

    if (dirX != 0)
        mapTempSprite.scroll(dirX * tileSize, 0);
    else if (dirY != 0)
        mapTempSprite.scroll(0, dirY * tileSize);

    for (int8_t i = 0; i < 2; ++i)
    {
        const int16_t tileToLoadX = startX + ((dirX == 0) ? i - 1 : 0);
        const int16_t tileToLoadY = startY + ((dirY == 0) ? i - 1 : 0);
        MapTile roundMapTile;
        roundMapTile.tilex = tileToLoadX;
        roundMapTile.tiley = tileToLoadY;
        roundMapTile.zoom  = Maps::zoomLevel;
        roundMapTile.lat   = Maps::tiley2lat(tileToLoadY, Maps::zoomLevel);
        roundMapTile.lon   = Maps::tilex2lon(tileToLoadX, Maps::zoomLevel);
        snprintf(roundMapTile.file, sizeof(roundMapTile.file), mapRenderFolder,
                 Maps::zoomLevel, tileToLoadX, tileToLoadY);

        // Calculate the grid coordinates in the mapTempSprite
        // Grid is based on tilesGrid (3 or 4)
        int16_t gx = (tileToLoadX - (int32_t)navTlTileX_);
        int16_t gy = (tileToLoadY - (int32_t)navTlTileY_);
        int16_t sx = gx * tileSize;
        int16_t sy = gy * tileSize;

        if (!mapTempSprite.drawPngFile(roundMapTile.file, sx, sy))
            mapTempSprite.fillRect(sx, sy, tileSize, tileSize, TFT_LIGHTGREY);
    }
}

/**
 * @brief Returns the LOD (Level of Detail) skip threshold in pixels for the given zoom level.
 *
 * @details Used by renderNavLineString() and renderNavPolygon() to skip coordinate pairs
 *          that are too close together to be visually relevant at the current zoom.
 *
 * @param zoom Current map zoom level.
 * @return Pixel distance threshold below which coordinates are skipped.
 */
static int16_t getLODThreshold(uint8_t zoom)
{
    if (zoom <= 12)
        return 3;
    if (zoom <= 14)
        return 2;
    return 1;
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
    static uint16_t lastInColor = 0;
    static float lastAmount = -1.0f;
    static uint16_t lastOutColor = 0;

    if (color == lastInColor && amount == lastAmount)
        return lastOutColor;

    uint16_t factor = (uint16_t)((1.0f - amount) * 256.0f);
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    r = static_cast<uint8_t>((r * factor) >> 8);
    g = static_cast<uint8_t>((g * factor) >> 8);
    b = static_cast<uint8_t>((b * factor) >> 8);
    
    lastInColor = color;
    lastAmount = amount;
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
 * @param map        Reference to the target TFT_eSprite where the polygon is rendered.
 * @param px         Array of X-coordinates for the vertices.
 * @param py         Array of Y-coordinates for the vertices.
 * @param numPoints  Total count of vertices across all rings.
 * @param color      16-bit (RGB565) color for the fill.
 * @param xOffset    Horizontal translation applied to the final drawing coordinates.
 * @param yOffset    Vertical translation applied to the final drawing coordinates.
 * @param ringCount  The number of independent rings (use 0 or 1 for simple polygons).
 * @param ringEnds   Array containing the end indices for each ring in the px/py arrays. 
 */
void Maps::fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount, const uint16_t* ringEnds)
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
    int startY = std::max(minY, -yOffset);
    int endY = std::min(maxY, (int)tileHeight - 1 - yOffset);

    if (startY > minY)
    {
        for (int y = minY; y < startY; y++)
        {
            int eIdx = edgeBuckets[y - minY];
            while (eIdx != -1)
            {
                int nextIdx = edgePool[eIdx].nextInBucket;
                edgePool[eIdx].xVal += edgePool[eIdx].slope * (startY - y);
                edgePool[eIdx].nextActive = activeHead;
                activeHead = eIdx;
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
        int eIdx = edgeBuckets[y - minY];
        while (eIdx != -1)
        {
            int nextIdx = edgePool[eIdx].nextInBucket;
            edgePool[eIdx].nextActive = activeHead;
            activeHead = eIdx;
            eIdx = nextIdx;
        }
        int* pCurrIdx = &activeHead;
        while (*pCurrIdx != -1)
        {
            if (edgePool[*pCurrIdx].yMax <= y)
                *pCurrIdx = edgePool[*pCurrIdx].nextActive;
            else
                pCurrIdx = &(edgePool[*pCurrIdx].nextActive);
        }
        if (activeHead == -1)
            continue;

        int sorted = -1;
        int active = activeHead;
        while (active != -1)
        {
            int nextActive = edgePool[active].nextActive;
            if (sorted == -1 || edgePool[active].xVal < edgePool[sorted].xVal)
            {
                edgePool[active].nextActive = sorted;
                sorted = active;
            }
            else
            {
                int s = sorted;
                while (edgePool[s].nextActive != -1 && edgePool[edgePool[s].nextActive].xVal < edgePool[active].xVal)
                    s = edgePool[s].nextActive;
                edgePool[active].nextActive = edgePool[s].nextActive;
                edgePool[s].nextActive = active;
            }
            active = nextActive;
        }
        activeHead = sorted;

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
        for (int a = activeHead; a != -1; a = edgePool[a].nextActive)
            edgePool[a].xVal += edgePool[a].slope;
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
    const float n = static_cast<float>(1u << navLastZoom_);
    const float tx = (lon + 180.0f) / 360.0f * n;
    const float merc_n = calcMercatorN(lat);
    const float ty = (1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * n;
    px = static_cast<int16_t>((tx - navTlTileX_) * 256.0f);
    py = static_cast<int16_t>((ty - navTlTileY_) * 256.0f);
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
void Maps::drawThickLine(TFT_eSprite& map, int16_t x0, int16_t y0,
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
 * @brief Renders a NAVLineString (roads, paths, etc.) onto a sprite.
 *
 * @details This function decodes compressed vector data and draws it as a series of
 *          connected segments. It supports "casing" (drawing a slightly wider, darker
 *          background line to create an outline effect) and applies dynamic Level of
 *          Detail (LOD) filtering based on the current zoom level to optimize performance.
 *
 * @param ref Reference to the feature data, including coordinates and style.
 * @param map The target TFT_eSprite for rendering.
 * @param isCasing  If true, renders the line outline (wider and darkened).
 *                  If false, renders the main line body.
 */
void Maps::renderNavLineString(const FeatureRef& ref, TFT_eSprite& map, bool isCasing)
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
    const int16_t lodThreshold = getLODThreshold(navLastZoom_);

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
                drawThickLine(map, lastPx, lastPy, px, py, iWidth, color);
            }
        }
        lastPx = px;
        lastPy = py;
    }
}

/**
 * @brief Renders a NAV polygon (parks, water, buildings) onto a sprite.
 * 
 * @details This function processes encoded vector data to reconstruct polygon geometry, 
 *          including support for multiple rings (holes or multi-part polygons). It includes 
 *          coordinate simplification for performance and optional outline (casing) rendering 
 *          at high zoom levels.
 *
 * @param ref  Reference to the feature data, including vertex pointers, 
 *             colors, and styling metadata.
 * @param map  The target TFT_eSprite where the polygon and its outline will be drawn.
 */
void Maps::renderNavPolygon(const FeatureRef& ref, TFT_eSprite& map)
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
        {
            ringCount = 0;
        }
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
    const int16_t lodThreshold = getLODThreshold(navLastZoom_);

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
    if (ref.casing && navLastZoom_ >= 16)
    {
        uint16_t outlineColor = darkenRGB565(ref.color, 0.35f);
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

                map.drawLine(px[j], py[j], px[next], py[next], outlineColor);
            }
            ringStart = ringEnd;
        }
    }
}

/**
 * @brief Renders a NAV point (POI) as a filled circle.
 * 
 * @details Decodes the point's coordinates using ZigZag/VarInt, applies the tile offset, 
 *          and draws a circle at the resulting position if it falls within the tile bounds.
 * 
 * @param ref Reference to the point feature data and styling.
 * @param map The target sprite for rendering.
 */
void Maps::renderNavPoint(const FeatureRef& ref, TFT_eSprite& map)
{
    if (ref.coordCount == 0)
        return;
    uint8_t* p = ref.ptr;
    int32_t x = NavReader::decodeZigZag(NavReader::readVarInt(p));
    int32_t y = NavReader::decodeZigZag(NavReader::readVarInt(p));
    int16_t px = ref.tileOffsetX + (x >> 4);
    int16_t py = ref.tileOffsetY + (y >> 4);

    if (px >= 0 && px < (int)tileWidth && py >= 0 && py < (int)tileHeight)
        map.fillCircle(px, py, 3, ref.color);
}

/**
 * @brief Renders NAV text labels with collision detection.
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
void Maps::renderNavText(const FeatureRef& ref, TFT_eSprite& map, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels)
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
        placedLabels.push_back({(int16_t)lx, (int16_t)ly, (int16_t)tw, (int16_t)th});
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
    pendingTilesNotEmpty_ = true;
}

/**
 * @brief Initializes and prepares viewport for rendering.
 *
 * @param centerLat Latitude of the viewport center.
 * @param centerLon Longitude of the viewport center.
 * @param zoom Target zoom level.
 * @param map Reference to the sprite used for rendering.
 * @return true if the viewport was successfully initialized.
 */
bool Maps::renderNavViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite& map)
{
    const uint32_t centerTileIdxX = lon2tilex(centerLon, zoom);
    const uint32_t centerTileIdxY = lat2tiley(centerLat, zoom);
    const int8_t gridOffset = tilesGrid / 2;
    navTlTileX_ = (float)(centerTileIdxX - gridOffset);
    navTlTileY_ = (float)(centerTileIdxY - gridOffset);
    bool zoomChanged = (zoom != navLastZoom_);
    navLastZoom_ = zoom;
    if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        redrawMap = true;
        pendingTiles.clear();
        pendingTilesNotEmpty_ = false;
        enqueueTileGrid(centerTileIdxX, centerTileIdxY, TILE_NAV);
        xSemaphoreGiveRecursive(mapMutex);
    }
    return true;
}

/**
 * @brief Looks up a NAV tile in the LRU cache, loading it from storage on miss.
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
uint8_t* Maps::navCacheLookupOrLoad(uint32_t tileX, uint32_t tileY, uint8_t zoom, size_t& outDataSize)
{
    uint32_t tileHash = (uint32_t(zoom) << 28) | (uint32_t(tileX & 0x3FFF) << 14) | uint32_t(tileY & 0x3FFF);
    int cacheIdx = -1;
    for (int i = 0; i < (int)navDataCache.size(); i++)
    {
        if (navDataCache[i].tileHash == tileHash)
        {
            cacheIdx = i;
            break;
        }
    }
    if (cacheIdx >= 0)
    {
        navDataCache[cacheIdx].lastAccess = ++cacheCounter;
        navDataCache[cacheIdx].isPinned = true;
        outDataSize = navDataCache[cacheIdx].size;
        return navDataCache[cacheIdx].data;
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
        for (int i = (int)navDataCache.size() - 1; i >= 0; i--)
        {
            if (!navDataCache[i].isPinned)
            {
                heap_caps_free(navDataCache[i].data);
                navDataCache.erase(navDataCache.begin() + i);
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

    if (navDataCache.size() >= NAV_DATA_CACHE_SIZE)
    {
        int lru = -1;
        for (int i = 0; i < (int)navDataCache.size(); i++)
        {
            if (!navDataCache[i].isPinned && (lru == -1 || navDataCache[i].lastAccess < navDataCache[lru].lastAccess))
                lru = i;
        }
        if (lru != -1)
        {
            heap_caps_free(navDataCache[lru].data);
            navDataCache.erase(navDataCache.begin() + lru);
        }
    }

    navDataCache.push_back({data, size, tileHash, ++cacheCounter, true});
    outDataSize = size;
    return data;
}

/**
 * @brief Decodes all features from a NAV tile buffer into featurePool and layers.
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
void Maps::navDecodeFeatures(const uint8_t* data, size_t dataSize, int16_t screenX, int16_t screenY, uint8_t zoom)
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
 * @brief Fetches and decodes a single NAV tile from cache or storage.
 *
 * @param tileX   The global X index of the tile.
 * @param tileY   The global Y index of the tile.
 * @param zoom    The current map zoom level.
 * @param screenX The horizontal pixel offset on the target sprite.
 * @param screenY The vertical pixel offset on the target sprite.
 * @param map     The target sprite (unused, kept for API compatibility).
 */
void Maps::renderNavTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map)
{
    size_t dataSize = 0;
    uint8_t* data = navCacheLookupOrLoad(tileX, tileY, zoom, dataSize);
    if (!data || dataSize < NAV_TILE_HDR_SIZE)
        return;
    navDecodeFeatures(data, dataSize, screenX, screenY, zoom);
}