/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.5
 * @date 2026-04
 */

#include "maps.hpp"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <climits>
#include <cstdint>
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
#include "globalGpxDef.h"

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern TrackVector trackData;
const char* TAG = "Maps";

/**
 * @brief Map Class constructor
 */
Maps::Maps() : navLastZoom_(0), 
               navNeedsRender_(true),
               navTlTileX_(-1), 
               navTlTileY_(-1)
{
    projBuf32X.reserve(MAX_POLYGON_POINTS);
    projBuf32Y.reserve(MAX_POLYGON_POINTS);
    decodedCoords.reserve(MAX_POLYGON_POINTS * 2);
    edgePool.reserve(MAX_POLYGON_POINTS);
    edgeBuckets.reserve(tileHeight);
    featurePool.reserve(MAX_FEATURE_POOL_SIZE);

    for (int i = 0; i < 16; i++)
        layers[i].reserve(MAX_FEATURE_POOL_SIZE / 4);

    ringEndsCache.reserve(MAX_POLYGON_POINTS);
    placedLabelsCache.reserve(512);
    navDataCache.reserve(NAV_DATA_CACHE_SIZE);
    mapMutex = xSemaphoreCreateRecursiveMutex();
    mapEventGroup = xEventGroupCreate();
    xTaskCreatePinnedToCore(mapRenderTask, "MapRenderTask", 8192, this, 2, &mapRenderTaskHandle, 0);
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
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
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
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
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
void Maps::coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY)
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
    Maps::mapTempSprite.loadFont("/spiffs/font.vlw");
    Maps::mapSprite.createSprite(mapWidth, mapHeight);
    Maps::mapBuffer = Maps::mapSprite.getBuffer();
    Maps::preloadSprite.deleteSprite();
    Maps::preloadSprite.createSprite(mapTileSize * 2, mapTileSize * 2);
    Maps::oldMapTile = {};
    Maps::currentMapTile = {};
    Maps::roundMapTile = {};
    Maps::navArrowPosition = {0, 0};
    Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};
}

/**
 * @brief Delete map sprites
 */
void Maps::deleteMapScrSprites()
{
    NavReader::closePack();
    Maps::preloadSprite.deleteSprite();
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
 */
void Maps::drawTrack(TFT_eSprite &map)
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
    trackNeedsRedraw = true;
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

        if (trackNeedsRedraw)
        {
            if (xSemaphoreTakeRecursive(mapMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                drawTrack(mapTempSprite);
                trackNeedsRedraw = false;
                Maps::redrawMap = true;
                xSemaphoreGiveRecursive(mapMutex);
            }
        }

        if (!zoomChanged && !tileChanged && !navNeedsRender_ && pendingTiles.empty())
            return;

        if (pendingTiles.size() > (tilesGrid * tilesGrid))
            return;

        Maps::isMapFound = renderNavViewport(baseLat, baseLon, zoom, Maps::mapTempSprite);
        navLastZoom_ = zoom;
        navNeedsRender_ = false;
        latLonToPixel(destLat, destLon, (int16_t&)wptPosX, (int16_t&)wptPosY);
        drawTrack(mapTempSprite);
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
            static const int8_t spiralOrder[9][2] = {{1,1}, {0,1}, {1,0}, {2,1}, {1,2}, {0,0}, {2,0}, {0,2}, {2,2}};
            for (int i = 0; i < 9; i++)
            {
                int gx = spiralOrder[i][0];
                int gy = spiralOrder[i][1];
                uint32_t tx = tlX + gx;
                uint32_t ty = tlY + gy;
                int16_t sx = gx * mapTileSize;
                int16_t sy = gy * mapTileSize;
                char tilePath[128];
                snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoom, tx, ty);

                if (mapTempSprite.drawPngFile(tilePath, sx, sy))
                {
                    if (tx == centerTileIdxX && ty == centerTileIdxY)
                        centerFound = true;

                    const tileBounds currentBounds = Maps::getTileBounds(tx, ty, zoom);
                    if (currentBounds.lat_min < Maps::totalBounds.lat_min)
                        Maps::totalBounds.lat_min = currentBounds.lat_min;
                    if (currentBounds.lat_max > Maps::totalBounds.lat_max)
                        Maps::totalBounds.lat_max = currentBounds.lat_max;
                    if (currentBounds.lon_min < Maps::totalBounds.lon_min)
                        Maps::totalBounds.lon_min = currentBounds.lon_min;
                    if (currentBounds.lon_max > Maps::totalBounds.lon_max)
                        Maps::totalBounds.lon_max = currentBounds.lon_max;
                }
                else
                {
                    mapTempSprite.fillRect(sx, sy, 256, 256, TFT_BLACK);
                    mapTempSprite.drawPngFile(noMapFile, sx + 256 / 2 - 50, sy + 256 / 2 - 50);
                }
            }
        }
        else
        {
            for (int gy = 0; gy < tilesGrid; gy++)
            {
                for (int gx = 0; gx < tilesGrid; gx++)
                {
                    uint32_t tx = tlX + gx;
                    uint32_t ty = tlY + gy;
                    int16_t sx = gx * mapTileSize;
                    int16_t sy = gy * mapTileSize;
                    char tilePath[128];
                    snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoom, tx, ty);

                    if (mapTempSprite.drawPngFile(tilePath, sx, sy))
                    {
                        if (tx == centerTileIdxX && ty == centerTileIdxY)
                            centerFound = true;

                        const tileBounds currentBounds = Maps::getTileBounds(tx, ty, zoom);
                        if (currentBounds.lat_min < Maps::totalBounds.lat_min)
                            Maps::totalBounds.lat_min = currentBounds.lat_min;
                        if (currentBounds.lat_max > Maps::totalBounds.lat_max)
                            Maps::totalBounds.lat_max = currentBounds.lat_max;
                        if (currentBounds.lon_min < Maps::totalBounds.lon_min)
                            Maps::totalBounds.lon_min = currentBounds.lon_min;
                        if (currentBounds.lon_max > Maps::totalBounds.lon_max)
                            Maps::totalBounds.lon_max = currentBounds.lon_max;
                    }
                    else
                    {
                        mapTempSprite.fillRect(sx, sy, 256, 256, TFT_BLACK);
                        mapTempSprite.drawPngFile(noMapFile, sx + 256 / 2 - 50, sy + 256 / 2 - 50);
                    }
                }
            }
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

        drawTrack(mapTempSprite);
        redrawMap = true;
        xEventGroupSetBits(mapEventGroup, MAP_EVENT_DONE);
    }
}

/**
 * @brief Background task for map rendering
 */
void Maps::mapRenderTask(void* pvParameters)
{
    Maps* instance = (Maps*)pvParameters;
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
                        instance->layers[i].clear();
                }

                bool aborted = false;
                while (!instance->pendingTiles.empty())
                {
                    PendingTile t = instance->pendingTiles.back();
                    instance->pendingTiles.pop_back();
                    if (t.type == TILE_NAV)
                    {
                        instance->renderNavTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                        xSemaphoreGiveRecursive(instance->mapMutex);
                        vTaskDelay(1);
                        if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                        {
                            aborted = true;
                            break;
                        }
                        if (instance->pendingTiles.size() >= (tilesGrid * tilesGrid))
                        {
                            aborted = true;
                            break; // We have the mutex, outer block will release it
                        }
                    }
                    else if (t.type == TILE_PNG)
                    {
                        instance->renderPngTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                        xSemaphoreGiveRecursive(instance->mapMutex);
                        vTaskDelay(1);
                        if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                        {
                            aborted = true;
                            break;
                        }
                        if (instance->pendingTiles.size() >= (tilesGrid * tilesGrid))
                        {
                            aborted = true;
                            break; // We have the mutex, outer block will release it
                        }
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
                                instance->mapTempSprite.endWrite();
                                xSemaphoreGiveRecursive(instance->mapMutex);
                                vTaskDelay(pdMS_TO_TICKS(2));
                                if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                                {
                                    aborted = true;
                                    break;
                                }
                                if (!instance->pendingTiles.empty())
                                {
                                    aborted = true;
                                    break;
                                }
                                instance->mapTempSprite.startWrite();
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

                    if (aborted) break;

                    // Pass 2: LineString bodies and Texts
                    for (uint16_t idx : layer)
                    {
                        if ((++loopCounter & 127) == 0)
                        {
                            uint32_t now = millis();
                            if (now - lastYield > 40)
                            {
                                instance->mapTempSprite.endWrite();
                                xSemaphoreGiveRecursive(instance->mapMutex);
                                vTaskDelay(pdMS_TO_TICKS(2));
                                if (xSemaphoreTakeRecursive(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE)
                                {
                                    aborted = true;
                                    break;
                                }
                                if (!instance->pendingTiles.empty())
                                {
                                    aborted = true;
                                    break;
                                }
                                instance->mapTempSprite.startWrite();
                                lastYield = millis();
                            }
                        }

                        const auto& feat = instance->featurePool[idx];
                        if (feat.geomType == NavGeomType::LineString && feat.casing)
                            instance->renderNavLineString(feat, instance->mapTempSprite, false);
                        else if (feat.geomType == NavGeomType::Text)
                            instance->renderNavText(feat, instance->mapTempSprite, instance->placedLabelsCache);
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
                extern void triggerMapRedraw();
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
        mapHeading = mapSet.mapRotationComp ? globalSensorData.heading : gps.gpsData.heading;
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
        
        // Pivot in large source sprite (GPS position)
        Maps::mapTempSprite.setPivot(gridOffset * mapTileSize + Maps::navArrowPosition.posX,
                                     gridOffset * mapTileSize + Maps::navArrowPosition.posY);
        
        // Pivot in small destination sprite (Center of viewport)
        Maps::mapSprite.setPivot(mapScrWidth / 2, mapScrHeight / 2);
        
        // Rotate and crop directly to mapSprite
        Maps::mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
    }
    else
    {
        // Manual panning: crop central part of grid adjusted by displayOffsetX/offsetY
        int16_t cropX = (tileWidth - mapScrWidth) / 2 + displayOffsetX;
        int16_t cropY = (tileHeight - mapScrHeight) / 2 + displayOffsetY;
        mapTempSprite.pushSprite(&mapSprite, -cropX, -cropY);
    }

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
}

/**
 * @brief Mark map for redraw
 */
void Maps::updateMap()
{
    Maps::oldMapTile = {};
    navNeedsRender_ = true;
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
    const int16_t preloadWidth = (dirX != 0) ? tileSize : tileSize * 2;
    const int16_t preloadHeight = (dirY != 0) ? tileSize : tileSize * 2;
    if (!preloadSprite.getBuffer())
        preloadSprite.createSprite(mapTileSize * 2, mapTileSize * 2);

    const int16_t startX = tileX + dirX;
    const int16_t startY = tileY + dirY;

    for (int8_t i = 0; i < 2; ++i)
    {
        const int16_t tileToLoadX = startX + ((dirX == 0) ? i - 1 : 0);
        const int16_t tileToLoadY = startY + ((dirY == 0) ? i - 1 : 0);
        float tileLon = (tileToLoadX / (1 << Maps::zoomLevel)) * 360.0f - 180.0f;
        float tileLat = 90.0f - (tileToLoadY / (1 << Maps::zoomLevel)) * 180.0f;
        Maps::roundMapTile = Maps::getMapTile(tileLon, tileLat, Maps::zoomLevel, tileToLoadX, tileToLoadY);
        const int16_t offsetX = (dirX != 0) ? i * tileSize : 0;
        const int16_t offsetY = (dirY != 0) ? i * tileSize : 0;
        bool foundTile = false;
        if (!mapSet.vectorMap)
            foundTile = preloadSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);
        if (!foundTile)
            preloadSprite.fillRect(offsetX, offsetY, tileSize, tileSize, TFT_LIGHTGREY);
    }

    if (dirX != 0)
    {
        mapTempSprite.scroll(dirX * tileSize, 0);
        const int16_t pushX = (dirX > 0) ? tileSize * 2 : 0;
        mapTempSprite.pushImage(pushX, 0, preloadWidth, preloadHeight, preloadSprite.frameBuffer(0));
    }
    else if (dirY != 0)
    {
        mapTempSprite.scroll(0, dirY * tileSize);
        const int16_t pushY = (dirY > 0) ? tileSize * 2 : 0;
        mapTempSprite.pushImage(0, pushY, preloadWidth, preloadHeight, preloadSprite.frameBuffer(0));
    }
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
    int bucketCount = maxY - minY + 1;
    edgeBuckets.assign(bucketCount, -1);
    uint16_t count = (ringCount == 0) ? 1 : ringCount;
    uint16_t defaultEnds[1] = { (uint16_t)numPoints };
    const uint16_t* ends = (ringEnds == nullptr) ? defaultEnds : ringEnds;
    int ringStart = 0;

    for (uint16_t r = 0; r < count; r++)
    {
        int ringEnd = ends[r];
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
        bool changed = false;
        int eIdx = edgeBuckets[y - minY];
        while (eIdx != -1)
        {
            int nextIdx = edgePool[eIdx].nextInBucket;
            edgePool[eIdx].nextActive = activeHead;
            activeHead = eIdx;
            eIdx = nextIdx;
            changed = true;
        }
        int* pCurrIdx = &activeHead;
        while (*pCurrIdx != -1)
        {
            if (edgePool[*pCurrIdx].yMax <= y)
            {
                *pCurrIdx = edgePool[*pCurrIdx].nextActive;
                changed = true;
            }
            else
                pCurrIdx = &(edgePool[*pCurrIdx].nextActive);
        }
        if (activeHead == -1)
            continue;

        if (changed)
        {
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
        }
        else
        {
            bool swapped = true;
            while (swapped)
            {
                swapped = false;
                int* pPrev = &activeHead;
                int curr = activeHead;
                while (curr != -1 && edgePool[curr].nextActive != -1)
                {
                    int next = edgePool[curr].nextActive;
                    if (edgePool[curr].xVal > edgePool[next].xVal)
                    {
                        edgePool[curr].nextActive = edgePool[next].nextActive;
                        edgePool[next].nextActive = curr;
                        *pPrev = next;
                        swapped = true;
                    }
                    pPrev = &((*pPrev == curr) ? edgePool[curr].nextActive : *pPrev);
                    curr = *pPrev;
                }
            }
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
                map.drawFastHLine(xStart, yy, xEnd - xStart, color);
            left = edgePool[right].nextActive;
        }
        for (int a = activeHead; a != -1; a = edgePool[a].nextActive)
            edgePool[a].xVal += edgePool[a].slope;
    }
}

/**
 * @brief Projects geographic coordinates (Latitude/Longitude) to local pixel coordinates.
 * 
 * @details This function performs a Web Mercator projection to convert WGS84 decimal degrees 
 *          into global tile coordinates based on the current zoom level (@p navLastZoom_). 
 *          It then transforms these into local pixel offsets relative to the top-left 
 *          tile of the current viewport (navTlTileX_, navTlTileY_).
 *  
 * @param lat  Latitude in decimal degrees 
 * @param lon  Longitude in decimal degrees 
 * @param px   Calculated horizontal pixel position relative to the current map view.
 * @param py   Calculated vertical pixel position relative to the current map view.
 */
void Maps::latLonToPixel(float lat, float lon, int16_t& px, int16_t& py)
{
    const float latRad = lat * (float)M_PI / 180.0f;
    const float n = static_cast<float>(1u << navLastZoom_);
    const float tx = (lon + 180.0f) / 360.0f * n;
    const float ty = (1.0f - logf(tanf(latRad) + 1.0f / cosf(latRad)) / (float)M_PI) / 2.0f * n;
    px = static_cast<int16_t>((tx - navTlTileX_) * 256.0f);
    py = static_cast<int16_t>((ty - navTlTileY_) * 256.0f);
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
    int16_t lodThreshold;

    if (navLastZoom_ <= 12)
        lodThreshold = 3;
    else if (navLastZoom_ <= 14)
        lodThreshold = 2;
    else
        lodThreshold = 1;

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
                if (widthF <= 1.1f)
                    map.drawLine(lastPx, lastPy, px, py, color);
                else
                    map.drawWideLine(lastPx, lastPy, px, py, widthF, color);
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
    ringEndsCache.clear();
    if ((size_t)(p_rings - ref.ptr) < ref.payloadSize)
    {
        ringCount = p_rings[0] | (p_rings[1] << 8);
        if (ringCount > 0)
        {
            uint8_t* p_curr_ring = p_rings + 2;
            for (int r = 0; r < (int)ringCount; r++)
            {
                ringEndsCache.push_back(p_curr_ring[0] | (p_curr_ring[1] << 8));
                p_curr_ring += 2;
            }
            ringEndsPtr = ringEndsCache.data();
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
    int16_t lodThreshold;
    if (navLastZoom_ <= 12)
        lodThreshold = 3;
    else if (navLastZoom_ <= 14)
        lodThreshold = 2;
    else
        lodThreshold = 1;

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

    bool fastCollision = false;
    for (const auto& r : placedLabels)
    {
        if (elx - PAD < r.x + r.w && elx + estimatedWidth + PAD > r.x && 
            ely - PAD < r.y + r.h && ely + th + PAD > r.y)
        {
            fastCollision = true;
            break;
        }
    }

    // Only if fast check is safe, we calculate precise width
    int tw = map.textWidth(textBuf);
    int lx = px - tw / 2;
    int ly = py - th;

    if (lx + tw < 0 || lx >= (int)tileWidth || ly + th < 0 || ly >= (int)tileHeight)
        return;

    // If fast check already found a collision, we can double-check with precise width
    // or just skip to save CPU if the heuristic was close enough.
    // Let's do a precise check now that we have the real 'tw'
    bool preciseCollision = false;
    for (const auto& r : placedLabels)
    {
        if (abs(ly - r.y) > th + PAD * 2)
            continue;

        if (lx - PAD < r.x + r.w && lx + tw + PAD > r.x && ly - PAD < r.y + r.h && ly + th + PAD > r.y)
        {
            preciseCollision = true;
            break;
        }
    }

    if (preciseCollision)
        return;

    map.setTextColor(ref.color);
    map.setTextDatum(lgfx::top_center);
    map.drawString(textBuf, px, ly);
    map.setTextDatum(lgfx::top_left);

    if (placedLabels.size() < placedLabels.capacity())
        placedLabels.push_back({(int16_t)lx, (int16_t)ly, (int16_t)tw, (int16_t)th});
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
        if (tilesGrid == 3)
        {
            static const int8_t spiralOrder[9][2] = {{0,0}, {2,0}, {0,2}, {2,2}, {0,1}, {1,0}, {2,1}, {1,2}, {1,1}};
            for (int i = 0; i < 9; i++)
            {
                int dx = spiralOrder[i][0];
                int dy = spiralOrder[i][1];
                pendingTiles.push_back({(uint32_t)(centerTileIdxX - gridOffset + dx), (uint32_t)(centerTileIdxY - gridOffset + dy), (int16_t)(dx * 256), (int16_t)(dy * 256), TILE_NAV});
            }
        }
        else
        {
            for (int dy = 0; dy < tilesGrid; dy++)
            {
                for (int dx = 0; dx < tilesGrid; dx++)
                {
                    pendingTiles.push_back({(uint32_t)(centerTileIdxX - gridOffset + dx), (uint32_t)(centerTileIdxY - gridOffset + dy), (int16_t)(dx * 256), (int16_t)(dy * 256), TILE_NAV});
                }
            }
        }
        xSemaphoreGiveRecursive(mapMutex);
    }
    return true;
}

/**
 * @brief Fetches and decodes a single NAV tile from cache or storage.
 * 
 * @details Manages a LRU (Least Recently Used) cache in PSRAM for tile data. It opens 
 *          the corresponding zoom-level pack file, locates the tile via hash, and 
 *          extracts features into the featurePool. Includes view-frustum culling 
 *          and Level of Detail (LOD) filtering to skip features that are too small 
 *          or off-screen.
 * 
 * @param tileX The global X index of the tile.
 * @param tileY The global Y index of the tile.
 * @param zoom  The current map zoom level.
 * @param screenX The horizontal pixel offset on the target sprite.
 * @param screenY The vertical pixel offset on the target sprite.
 * @param map The target sprite for metadata updates (timing/stats).
 */
void Maps::renderNavTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map)
{
    uint64_t tStart = esp_timer_get_time();
    uint32_t tileHash = (uint32_t(zoom) << 28) | (uint32_t(tileX & 0x3FFF) << 14) | uint32_t(tileY & 0x3FFF);
    uint8_t* data = nullptr;
    size_t dataSize = 0;
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
        data = navDataCache[cacheIdx].data;
        dataSize = navDataCache[cacheIdx].size;
        navDataCache[cacheIdx].lastAccess = ++cacheCounter;
        navDataCache[cacheIdx].isPinned = true;
        cacheHits++;
    }
    else
    {
        cacheMisses++;
        if (!NavReader::openPack(zoom))
            return;
        uint32_t offset;
        uint32_t size;
        if (!NavReader::findTileInPack(tileX, tileY, offset, size))
            return;
        data = (uint8_t*)heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM);
        if (!data)
        {
            for (int i = (int)navDataCache.size()-1; i >= 0; i--)
                if (!navDataCache[i].isPinned) { heap_caps_free(navDataCache[i].data); navDataCache.erase(navDataCache.begin() + i); }
            data = (uint8_t*)heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM);
            if (!data)
                return;
        }
        storage.seek(NavReader::packFile, offset, SEEK_SET);
        if (storage.read(NavReader::packFile, data, size) != size)
        {
            heap_caps_free(data);
            return;
        }
        dataSize = size;
        if (navDataCache.size() >= NAV_DATA_CACHE_SIZE)
        {
            int lru = -1;
            for (int i = 0; i < (int)navDataCache.size(); i++)
                if (!navDataCache[i].isPinned && (lru == -1 || navDataCache[i].lastAccess < navDataCache[lru].lastAccess)) lru = i;
            if (lru != -1) { heap_caps_free(navDataCache[lru].data); navDataCache.erase(navDataCache.begin() + lru); }
        }
        navDataCache.push_back({data, size, tileHash, ++cacheCounter, true});
    }

    if (dataSize < 22)
        return;
    uint16_t feature_count;
    memcpy(&feature_count, data + 4, 2);
    uint8_t* p = data + 22;
    for (uint16_t i = 0; i < feature_count; i++)
    {
        if (p + 13 > data + dataSize)
            break;
        uint8_t geomType = p[0];
        uint8_t zp = p[3];
        uint8_t wp = p[4];
        uint8_t bx1 = p[5];
        uint8_t by1 = p[6];
        uint8_t bx2 = p[7];
        uint8_t by2 = p[8];
        uint16_t colorRgb565;
        uint16_t cc;
        uint16_t ps;
        memcpy(&colorRgb565, p + 1, 2);
        memcpy(&cc, p + 9, 2);
        memcpy(&ps, p + 11, 2);
        if (p + 13 + ps > data + dataSize)
            break;
        if ((zp >> 4) <= zoom)
        {
            if (screenX + bx2 < 0 || screenX + bx1 > (int)tileWidth || screenY + by2 < 0 || screenY + by1 > (int)tileHeight)
            {
                p += 13 + ps;
                continue;
            }
            int16_t dimX = bx2 - bx1;
            int16_t dimY = by2 - by1;
            uint8_t minDim = (zoom >= 9 && zoom <= 11) ? 3 : 1;
            if ((geomType == (uint8_t)NavGeomType::Polygon || geomType == (uint8_t)NavGeomType::LineString) && dimX < minDim && dimY < minDim)
            {
                p += 13 + ps;
                continue;
            }
            if (featurePool.size() < MAX_FEATURE_POOL_SIZE)
            {
                uint16_t poolIdx = (uint16_t)featurePool.size();
                featurePool.push_back({p + 13, (NavGeomType)geomType, ps, cc, screenX, screenY, colorRgb565, (uint8_t)(wp & 0x7F), (wp & 0x80) != 0, bx1, by1, bx2, by2, (uint8_t)(zp & 0x0F)});
                uint8_t priority = zp & 0x0F;
                if (priority < 16)
                    layers[priority].push_back(poolIdx);
            }
        }
        p += 13 + ps;
    }
}