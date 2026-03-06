/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.4
 * @date 2025-12
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
 * @details Initializes the Maps class with default polygon filling enabled and reserves memory in Internal RAM.
 */
Maps::Maps() : fillPolygons(true),
               navLastZoom_(0), navNeedsRender_(true),
               navTlTileX_(-1), navTlTileY_(-1)
{
    projBuf32X.reserve(MAX_POLYGON_POINTS);
    projBuf32Y.reserve(MAX_POLYGON_POINTS);
    decodedCoords.reserve(MAX_POLYGON_POINTS * 2);
    edgePool.reserve(MAX_POLYGON_POINTS);
    edgeBuckets.reserve(tileHeight);
    featurePool.reserve(MAX_FEATURE_POOL_SIZE);
    for (int i = 0; i < 16; i++) layers[i].reserve(1024);
    ringEndsCache.reserve(1024);
    placedLabelsCache.reserve(1024);
    navDataCache.reserve(NAV_DATA_CACHE_SIZE);
    mapMutex = xSemaphoreCreateMutex();
    mapEventGroup = xEventGroupCreate();
    xTaskCreatePinnedToCore(mapRenderTask, "MapRenderTask", 16384, this, 1, &mapRenderTaskHandle, 0);
}

/**
 * @brief Get pixel X position from longitude
 * @param f_lon Longitude coordinate.
 * @param zoom Zoom level.
 * @param tileSize Size of the map tile in pixels.
 * @return Pixel X position within the tile.
 */
uint16_t Maps::lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize)
{
    return static_cast<uint16_t>(((f_lon + 180.0f) / 360.0f * (1 << zoom) * tileSize)) % tileSize;
}

/**
 * @brief Get pixel Y position from latitude
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
    float scale = (1 << zoom) * tileSize;
    return static_cast<uint16_t>(((1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * scale)) % tileSize;
}

/**
 * @brief Get TileX for OpenStreetMap files
 * @param f_lon Longitude coordinate.
 * @param zoom Zoom level.
 * @return X tile index.
 */
uint32_t Maps::lon2tilex(float f_lon, uint8_t zoom)
{
    float rawTile = (f_lon + 180.0f) / 360.0f * (1 << zoom);
    rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}

/**
 * @brief Get TileY for OpenStreetMap files
 * @param f_lat Latitude coordinate.
 * @param zoom Zoom level.
 * @return Y tile index.
 */
uint32_t Maps::lat2tiley(float f_lat, uint8_t zoom)
{
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
    float rawTile = (1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * (1 << zoom);
    rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}

/**
 * @brief Get Longitude from tile X
 * @param tileX Tile X index.
 * @param zoom Zoom level.
 * @return Longitude coordinate.
 */
float Maps::tilex2lon(uint32_t tileX, uint8_t zoom)
{
    return static_cast<float>(tileX) * 360.0f / (1 << zoom) - 180.0f;
}

/**
 * @brief Get Latitude from tile Y
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
 * @param lon Longitude coordinate.
 * @param lat Latitude coordinate.
 * @param zoomLevel Zoom level.
 * @param offsetX Tile offset X.
 * @param offsetY Tile offset Y.
 * @return MapTile structure.
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
 * @param tileX Tile X coordinate.
 * @param tileY Tile Y coordinate.
 * @param zoom Zoom level.
 * @return tileBounds structure.
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
 * @param lat Latitude to check.
 * @param lon Longitude to check.
 * @param bound Map bounds.
 * @return true if inside bounds.
 */
bool Maps::isCoordInBounds(float lat, float lon, tileBounds bound)
{
    return (lat >= bound.lat_min && lat <= bound.lat_max &&
            lon >= bound.lon_min && lon <= bound.lon_max);
}

/**
 * @brief Convert GPS Coordinates to screen position
 * @param lon Longitude.
 * @param lat Latitude.
 * @param zoomLevel Zoom level.
 * @param tileSize Tile size.
 * @return ScreenCoord structure.
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
 * @param lat Latitude.
 * @param lon Longitude.
 * @param bound Tile boundaries.
 * @param pixelX Output X pixel.
 * @param pixelY Output Y pixel.
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
 * @param map Target sprite.
 */
void Maps::showNoMap(TFT_eSprite &map)
{
    map.drawPngFile(noMapFile, (Maps::mapScrWidth / 2) - 50, (Maps::mapScrHeight / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (Maps::mapScrWidth / 2), (Maps::mapScrHeight >> 1) + 65, &fonts::DejaVu18);
}

/**
 * @brief Initialize map sprites and variables
 * @param mapHeight Screen height.
 * @param mapWidth Screen width.
 */
void Maps::initMap(uint16_t mapHeight, uint16_t mapWidth)
{
    Maps::mapScrHeight = mapHeight;
    Maps::mapScrWidth = mapWidth;
    Maps::mapTempSprite.createSprite(Maps::tileWidth, Maps::tileHeight);
    Maps::mapSprite.createSprite(Maps::tileWidth, Maps::tileHeight);
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
 * @brief Create map sprites
 */
void Maps::createMapScrSprites()
{
    Maps::mapBuffer = Maps::mapSprite.getBuffer();
}

/**
 * @brief Redraw the current track on the map sprite.
 * @param map Target sprite.
 */
void Maps::drawTrack(TFT_eSprite &map)
{
    for (size_t i = 1; i < trackData.size(); ++i)
    {
        const auto &p1 = trackData[i - 1];
        const auto &p2 = trackData[i];
        int16_t x1, y1, x2, y2;
        latLonToPixel(p1.lat, p1.lon, x1, y1);
        latLonToPixel(p2.lat, p2.lon, x2, y2);
        if ((x1 >= 0 && x1 < tileWidth && y1 >= 0 && y1 < tileHeight) ||
            (x2 >= 0 && x2 < tileWidth && y2 >= 0 && y2 < tileHeight))
            map.drawWideLine(x1, y1, x2, y2, 3, TFT_BLUE);
    }
}

/**
 * @brief Force a redraw of the track on the next map update.
 */
void Maps::redrawTrack()
{
    trackNeedsRedraw = true;
}

/**
 * @brief Generate the map based on current position and zoom
 * @param zoom Current zoom level.
 */
void Maps::generateMap(uint8_t zoom)
{
    if (zoom != Maps::zoomLevel)
    {
        Maps::zoomLevel = zoom;
        // Sync currentMapTile with new zoom to avoid coordinate jumps in panMap
        Maps::currentMapTile.zoom = zoom;
        Maps::currentMapTile.tilex = Maps::lon2tilex(Maps::currentMapTile.lon, zoom);
        Maps::currentMapTile.tiley = Maps::lat2tiley(Maps::currentMapTile.lat, zoom);
        resetScrollState();
    }

    const float lat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
    const float lon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

    if (mapSet.vectorMap)
    {
        const double latRad = (double)lat * M_PI / 180.0;
        const double n = pow(2.0, (double)zoom);
        const int centerTileIdxX = (int)floorf((float)((lon + 180.0) / 360.0 * n));
        const int centerTileIdxY = (int)floorf((float)((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n));
        
        const int8_t gridOffset = tilesGrid / 2;
        const int32_t currentTlX = (int32_t)centerTileIdxX - gridOffset;
        const int32_t currentTlY = (int32_t)centerTileIdxY - gridOffset;
        
        bool zoomChanged = (zoom != navLastZoom_);
        bool tileChanged = (currentTlX != (int32_t)navTlTileX_ || currentTlY != (int32_t)navTlTileY_);
        
        if (trackNeedsRedraw)
        {
            if (xSemaphoreTake(mapMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                drawTrack(mapTempSprite);
                trackNeedsRedraw = false;
                Maps::redrawMap = true;
                xSemaphoreGive(mapMutex);
            }
        }
        if (!zoomChanged && !tileChanged && !navNeedsRender_ && pendingTiles.empty()) return;
        if (pendingTiles.size() > 9) return;
        Maps::isMapFound = renderNavViewport(lat, lon, zoom, Maps::mapTempSprite);
        navLastZoom_ = zoom;
        navNeedsRender_ = false;
        latLonToPixel(destLat, destLon, (int16_t&)wptPosX, (int16_t&)wptPosY);
        drawTrack(mapTempSprite);
        Maps::redrawMap = true;
        return;
    }

    const uint32_t centerTileIdxX = lon2tilex(lon, zoom);
    const uint32_t centerTileIdxY = lat2tiley(lat, zoom);

    if (centerTileIdxX != Maps::oldMapTile.tilex || centerTileIdxY != Maps::oldMapTile.tiley || zoom != Maps::oldMapTile.zoom)
    {
        Maps::oldMapTile.tilex = centerTileIdxX;
        Maps::oldMapTile.tiley = centerTileIdxY;
        Maps::oldMapTile.zoom = zoom;

        const int8_t gridOffset = tilesGrid / 2;
        const int32_t tlX = (int32_t)centerTileIdxX - gridOffset;
        const int32_t tlY = (int32_t)centerTileIdxY - gridOffset;

        // Update top-left for track drawing (latLonToPixel)
        navTlTileX_ = (float)tlX;
        navTlTileY_ = (float)tlY;
        navLastZoom_ = zoom;

        Maps::mapTempSprite.fillSprite(TFT_WHITE);
        Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};
        bool centerFound = false;

        static const int8_t spiralOrder[9][2] = {{1,1}, {0,1}, {1,0}, {2,1}, {1,2}, {0,0}, {2,0}, {0,2}, {2,2}};
        for (int i = 0; i < 9; i++)
        {
            int gx = spiralOrder[i][0], gy = spiralOrder[i][1];
            uint32_t tx = tlX + gx;
            uint32_t ty = tlY + gy;
            int16_t sx = gx * mapTileSize;
            int16_t sy = gy * mapTileSize;

            char tilePath[128];
            snprintf(tilePath, sizeof(tilePath), mapRenderFolder, zoom, tx, ty);
            
            if (mapTempSprite.drawPngFile(tilePath, sx, sy))
            {
                if (tx == centerTileIdxX && ty == centerTileIdxY) centerFound = true;
                
                const tileBounds currentBounds = Maps::getTileBounds(tx, ty, zoom);
                if (currentBounds.lat_min < Maps::totalBounds.lat_min) Maps::totalBounds.lat_min = currentBounds.lat_min;
                if (currentBounds.lat_max > Maps::totalBounds.lat_max) Maps::totalBounds.lat_max = currentBounds.lat_max;
                if (currentBounds.lon_min < Maps::totalBounds.lon_min) Maps::totalBounds.lon_min = currentBounds.lon_min;
                if (currentBounds.lon_max > Maps::totalBounds.lon_max) Maps::totalBounds.lon_max = currentBounds.lon_max;
            }
            else
            {
                mapTempSprite.fillRect(sx, sy, 256, 256, TFT_BLACK);
                mapTempSprite.drawPngFile(noMapFile, sx + 256 / 2 - 50, sy + 256 / 2 - 50);
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
        Maps::redrawMap = true;
        xEventGroupSetBits(mapEventGroup, MAP_EVENT_DONE);
    }
}

/**
 * @brief Background task for tile rendering on Core 0.
 * @details Monitors the pendingTiles queue and renders both NAV and PNG tiles asynchronously.
 *          Uses mapMutex to ensure thread-safe access to the mapTempSprite.
 * @param pvParameters Pointer to the Maps instance.
 */
void Maps::mapRenderTask(void* pvParameters)
{
    Maps* instance = (Maps*)pvParameters;
    uint8_t lastZoom = 0;
    while (1)
    {
        if (!instance->pendingTiles.empty())
        {
            if (xSemaphoreTake(instance->mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
            {
                bool fullReset = (instance->zoomLevel != lastZoom) || (instance->pendingTiles.size() >= 9);
                lastZoom = instance->zoomLevel;
                if (fullReset)
                {
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_DONE | MAP_EVENT_ERROR);
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_START);
                    instance->featurePool.clear();
                    instance->decodedCoords.clear();
                    for (int i = 0; i < 16; i++) instance->layers[i].clear();
                }
                while (!instance->pendingTiles.empty())
                {
                    PendingTile t = instance->pendingTiles.back();
                    instance->pendingTiles.pop_back();
                    if (t.type == TILE_NAV) instance->renderNavTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                    else if (t.type == TILE_PNG)
                    {
                        instance->renderPngTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                        // Release mutex between PNG tiles to keep UI/Scroll alive
                        xSemaphoreGive(instance->mapMutex);
                        vTaskDelay(1);
                        if (xSemaphoreTake(instance->mapMutex, pdMS_TO_TICKS(100)) != pdTRUE) break;
                    }
                }
                if (!mapSet.vectorMap)
                {
                    instance->drawTrack(instance->mapTempSprite);
                    instance->redrawMap = true;
                    xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                    xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                    xSemaphoreGive(instance->mapMutex);
                    continue;
                }
                instance->placedLabelsCache.clear();
                instance->totalFeaturesDrawn = (uint32_t)instance->featurePool.size();
                uint64_t tDrawStart = esp_timer_get_time();
                instance->mapTempSprite.startWrite();
                for (int i = 0; i < 16; i++)
                {
                    if (instance->layers[i].empty()) continue;
                    uint16_t featCount = 0;
                    uint8_t pass = (i == 15) ? 2 : 1;
                    for (uint16_t idx : instance->layers[i])
                    {
                        if ((++featCount & 63) == 0) esp_task_wdt_reset();
                        instance->renderNavFeature(instance->featurePool[idx], instance->mapTempSprite, pass, instance->placedLabelsCache);
                    }
                    instance->mapTempSprite.endWrite();
                    vTaskDelay(1);
                    instance->mapTempSprite.startWrite();
                    esp_task_wdt_reset();
                }
                instance->mapTempSprite.endWrite();
                instance->totalDrawTime = esp_timer_get_time() - tDrawStart;
                for (auto& entry : instance->navDataCache) entry.isPinned = false;
                instance->drawTrack(instance->mapTempSprite);
                instance->redrawMap = true;
                xEventGroupSetBits(instance->mapEventGroup, MAP_EVENT_DONE);
                xEventGroupClearBits(instance->mapEventGroup, MAP_EVENT_START);
                xSemaphoreGive(instance->mapMutex);
                uint64_t endTime = esp_timer_get_time();
                ESP_LOGI(TAG, "Zoom: %u | Full Render: %llu ms | Load: %llu ms | Proc: %llu ms | Draw: %llu ms | Features: %u | Cache H/M: %u/%u | PSRAM: %u KB", 
                         instance->zoomLevel, (endTime - instance->viewportStartTime) / 1000,
                         instance->totalLoadTime / 1000, instance->totalProcessTime / 1000,
                         instance->totalDrawTime / 1000, instance->totalFeaturesDrawn,
                         instance->cacheHits, instance->cacheMisses, ESP.getFreePsram() / 1024);
            }
        }
        vTaskDelay(1);
    }
}

/**
 * @brief Render a single PNG tile at a specific sprite position.
 * @details Constructs the file path and uses drawPngFile to render the tile. 
 *          Clears the area with black/no-map if the file is not found.
 * @param tileX Tile X coordinate.
 * @param tileY Tile Y coordinate.
 * @param zoom Zoom level.
 * @param screenX X position in the target sprite.
 * @param screenY Y position in the target sprite.
 * @param map Target sprite.
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
 * @brief Display the map on screen with rotation
 */
void Maps::displayMap()
{
    if (!Maps::isMapFound)
    {
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0);
        return;
    }

    if (xSemaphoreTake(mapMutex, pdMS_TO_TICKS(50)) != pdTRUE)
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
        Maps::mapTempSprite.setPivot(gridOffset * mapTileSize + Maps::navArrowPosition.posX,
                                     gridOffset * mapTileSize + Maps::navArrowPosition.posY);
        Maps::mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
    }
    else
    {
        // Universal fast transfer using direct buffer pointer
        mapSprite.pushImage(0, 0, tileWidth, tileHeight, (uint16_t*)mapTempSprite.getBuffer());
    }
    tft.endWrite();

    xSemaphoreGive(mapMutex);
}

/**
 * @brief Set waypoint coordinates
 * @param wptLat Latitude.
 * @param wptLon Longitude.
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
 * @param dx X offset.
 * @param dy Y offset.
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
 * @param lat Latitude.
 * @param lon Longitude.
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
 * @brief Reset all scroll offsets and counters
 */
void Maps::resetScrollState()
{
    Maps::tileX = 0;
    Maps::tileY = 0;
    Maps::lastTileX = 0;
    Maps::lastTileY = 0;
    Maps::offsetX = 0;
    Maps::offsetY = 0;
    Maps::velocityX = 0;
    Maps::velocityY = 0;
}

/**
 * @brief Smooth scroll the map with inertia
 * @param dx Delta X.
 * @param dy Delta Y.
 */
void Maps::scrollMap(int16_t dx, int16_t dy)
{
    if (dx != 0 || dy != 0)
    {
        // Apply elastic resistance if we are beyond the threshold (128px)
        // This helps both NAV (gives time to Core 0) and PNG (smoothes the jump)
        const int16_t softLimit = 128;
        if (abs(Maps::offsetX) > softLimit && ((dx > 0 && Maps::offsetX > 0) || (dx < 0 && Maps::offsetX < 0)))
            dx /= 2;
        if (abs(Maps::offsetY) > softLimit && ((dy > 0 && Maps::offsetY > 0) || (dy < 0 && Maps::offsetY < 0)))
            dy /= 2;

        Maps::offsetX += dx;
        Maps::offsetY += dy;
        Maps::followGps = false;
    }

    const int16_t maxOffsetX = (tileWidth - mapScrWidth) / 2 - 10;
    const int16_t maxOffsetY = (tileHeight - mapScrHeight) / 2 - 10;

    if (Maps::offsetX > maxOffsetX)
        Maps::offsetX = maxOffsetX;
    if (Maps::offsetX < -maxOffsetX)
        Maps::offsetX = -maxOffsetX;
    if (Maps::offsetY > maxOffsetY)
        Maps::offsetY = maxOffsetY;
    if (Maps::offsetY < -maxOffsetY)
        Maps::offsetY = -maxOffsetY;

    Maps::scrollUpdated = false;
#ifdef T4_S3
    const int16_t threshold = 160;
#else
    const int16_t threshold = 128;
#endif
    const int16_t tileSize = Maps::mapTileSize;

    if (Maps::offsetX <= -threshold)
    {
        Maps::tileX--;
        Maps::offsetX += tileSize;
        Maps::scrollUpdated = true;
    }
    else if (Maps::offsetX >= threshold)
    {
        Maps::tileX++;
        Maps::offsetX -= tileSize;
        Maps::scrollUpdated = true;
    }

    if (Maps::offsetY <= -threshold)
    {
        Maps::tileY--;
        Maps::offsetY += tileSize;
        Maps::scrollUpdated = true;
    }
    else if (Maps::offsetY >= threshold)
    {
        Maps::tileY++;
        Maps::offsetY -= tileSize;
        Maps::scrollUpdated = true;
    }

    if (Maps::scrollUpdated)
    {
        const int8_t deltaTileX = Maps::tileX - Maps::lastTileX;
        const int8_t deltaTileY = Maps::tileY - Maps::lastTileY;
        Maps::panMap(deltaTileX, deltaTileY);
        if (!mapSet.vectorMap)
            Maps::preloadTiles(deltaTileX, deltaTileY);
        
        // Reset telemetry for this scroll movement
        viewportStartTime = esp_timer_get_time();
        totalLoadTime = 0;
        totalProcessTime = 0;
        totalDrawTime = 0;
        totalFeaturesDrawn = 0;
        cacheHits = 0;
        cacheMisses = 0;

        generateMap(zoomLevel);
        Maps::lastTileX = Maps::tileX;
        Maps::lastTileY = Maps::tileY;
        Maps::redrawMap = true;
    }
}

/**
 * @brief Preload PNG tiles in scroll direction
 * @param dirX Direction X.
 * @param dirY Direction Y.
 */
void Maps::preloadTiles(int8_t dirX, int8_t dirY)
{
    const int16_t tileSize = mapTileSize;
    const int16_t preloadWidth  = (dirX != 0) ? tileSize : tileSize * 2;
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
 * @param color RGB565 color.
 * @param amount Darkening fraction.
 * @return Darkened color.
 */
uint16_t Maps::darkenRGB565(const uint16_t color, const float amount)
{
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    r = static_cast<uint8_t>(r * (1.0f - amount));
    g = static_cast<uint8_t>(g * (1.0f - amount));
    b = static_cast<uint8_t>(b * (1.0f - amount));
    return ((r << 11) | (g << 5) | b);
}

/**
 * @brief Fill a polygon using AEL scanline algorithm
 * @param map Target sprite.
 * @param px X coordinates.
 * @param py Y coordinates.
 * @param numPoints Vertex count.
 * @param color Fill color.
 * @param xOffset X offset.
 * @param yOffset Y offset.
 * @param ringCount Multi-ring count.
 * @param ringEnds Cumulative ring indices.
 */
void Maps::fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount, const uint16_t* ringEnds)
{
    if (numPoints < 3) return;
    int minY = INT_MAX, maxY = INT_MIN;
    for (int i = 0; i < numPoints; i++)
    {
        if (py[i] < minY) minY = py[i];
        if (py[i] > maxY) maxY = py[i];
    }
    if (maxY < 0 || minY >= (int)tileHeight) return;
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
            int x1 = px[ringStart + i], y1 = py[ringStart + i];
            int x2 = px[ringStart + next], y2 = py[ringStart + next];
            if (y1 == y2) continue;
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
        if (activeHead == -1) continue;
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
            if (xStart < 0) xStart = 0;
            if (xEnd > (int)tileWidth) xEnd = (int)tileWidth;
            if (xEnd > xStart)
                map.drawFastHLine(xStart, yy, xEnd - xStart, color);
            left = edgePool[right].nextActive;
        }
        for (int a = activeHead; a != -1; a = edgePool[a].nextActive)
            edgePool[a].xVal += edgePool[a].slope;
    }
}

/**
 * @brief Project Lat/Lon coordinates to NAV viewport pixels
 * @param lat Latitude coordinate.
 * @param lon Longitude coordinate.
 * @param px Output X pixel coordinate.
 * @param py Output Y pixel coordinate.
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
 * @brief Render a NAV LineString feature.
 * @param ref FeatureRef structure.
 * @param map Target sprite.
 * @param isCasing True if rendering bridge borders.
 */
void Maps::renderNavLineString(const FeatureRef& ref, TFT_eSprite& map, bool isCasing)
{
    if (ref.coordCount < 2)
        return;
    
    decodedCoords.resize(ref.coordCount * 2);
    int16_t* coords = decodedCoords.data();
    uint8_t* p = ref.ptr;
    int32_t curX = 0, curY = 0;
    
    for (uint16_t i = 0; i < ref.coordCount; i++)
    {
        curX += NavReader::decodeZigZag(NavReader::readVarInt(p));
        curY += NavReader::decodeZigZag(NavReader::readVarInt(p));
        coords[i * 2] = ref.tileOffsetX + (curX >> 4);
        coords[i * 2 + 1] = ref.tileOffsetY + (curY >> 4);
    }

    uint16_t color = ref.color;
    float widthF = (ref.width == 0 ? 2 : ref.width) / 2.0f;
    if (isCasing)
    {
        color = darkenRGB565(color, 0.3f);
        widthF += 1.0f;
    }
    int16_t lastPx = -32768, lastPy = -32768;
    for (uint16_t i = 0; i < ref.coordCount; i++)
    {
        int16_t px = coords[i * 2], py = coords[i * 2 + 1];
        if (i > 0)
        {
            // Skip points that are too close (LOD filtering)
            if (abs(px - lastPx) < 1 && abs(py - lastPy) < 1) continue;
            if (!((px < 0 && lastPx < 0) || (px >= (int)tileWidth && lastPx >= (int)tileWidth) || (py < 0 && lastPy < 0) || (py >= (int)tileHeight && lastPy >= (int)tileHeight)))
            {
                if (widthF <= 1.0f) map.drawLine(lastPx, lastPy, px, py, color);
                else map.drawWideLine(lastPx, lastPy, px, py, widthF, color);
            }
        }
        lastPx = px; lastPy = py;
    }
}

/**
 * @brief Render a NAV Polygon feature.
 * @param ref FeatureRef structure.
 * @param map Target sprite.
 */
void Maps::renderNavPolygon(const FeatureRef& ref, TFT_eSprite& map)
{
    if (ref.coordCount < 3 || ref.coordCount > MAX_POLYGON_POINTS)
        return;
    
    decodedCoords.resize(ref.coordCount * 2);
    int16_t* coords = decodedCoords.data();
    uint8_t* p = ref.ptr;
    int32_t curX = 0, curY = 0;
    
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
    
    projBuf32X.resize(ref.coordCount);
    projBuf32Y.resize(ref.coordCount);
    int minPx = INT_MAX, maxPx = INT_MIN, minPy = INT_MAX, maxPy = INT_MIN;
    int16_t lastX = -32768, lastY = -32768;
    uint16_t actualPoints = 0;
    for (size_t i = 0; i < ref.coordCount; i++)
    {
        int16_t curX = coords[i * 2], curY = coords[i * 2 + 1];
        if (ringCount == 0 && i > 0 && abs(curX - lastX) < 1 && abs(curY - lastY) < 1 && i < ref.coordCount - 1) continue;
        projBuf32X[actualPoints] = curX;
        projBuf32Y[actualPoints] = curY;
        if (curX < minPx) minPx = curX;
        if (curX > maxPx) maxPx = curX;
        if (curY < minPy) minPy = curY;
        if (curY > maxPy) maxPy = curY;
        lastX = curX; lastY = curY;
        actualPoints++;
    }
    if (maxPx < 0 || minPx >= (int)tileWidth || maxPy < 0 || minPy >= (int)tileHeight)
        return;
    int* px = projBuf32X.data();
    int* py = projBuf32Y.data();
    if (fillPolygons)
        fillPolygonGeneral(map, px, py, actualPoints, ref.color, 0, 0, ringCount, ringEndsPtr);
    if (ref.casing && navLastZoom_ >= 16)
    {
        uint16_t outlineColor = darkenRGB565(ref.color, 0.35f);
        int ringStart = 0;
        uint16_t numRings = (ringCount > 0) ? ringCount : 1;
        for (uint16_t r = 0; r < numRings; r++)
        {
            uint16_t ringEnd = (ringEndsPtr && r < ringCount) ? ringEndsPtr[r] : actualPoints;
            if (ringEnd > actualPoints)
                ringEnd = actualPoints;
            for (uint16_t j = ringStart; j < ringEnd; j++)
            {
                uint16_t next = (j + 1 < ringEnd) ? j + 1 : ringStart;
                map.drawLine(px[j], py[j], px[next], py[next], outlineColor);
            }
            ringStart = ringEnd;
        }
    }
}

/**
 * @brief Render a NAV Point feature.
 * @param ref FeatureRef structure.
 * @param map Target sprite.
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
 * @brief Dispatch feature rendering based on pass and geometry type.
 * @param ref FeatureRef structure.
 * @param map Target sprite.
 * @param pass Current rendering pass (1-4).
 * @param placedLabels Persistent buffer for label collision detection.
 */
void Maps::renderNavFeature(const FeatureRef& ref, TFT_eSprite& map, uint8_t pass, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels)
{
    if (pass == 1)
    {
        if (ref.geomType == NavGeomType::Polygon)
            renderNavPolygon(ref, map);
        else if (ref.geomType == NavGeomType::Point)
            renderNavPoint(ref, map);
        else if (ref.geomType == NavGeomType::LineString)
            renderNavLineString(ref, map, ref.casing);
    }
    else if (pass == 2)
    {
        if (ref.geomType == NavGeomType::LineString && ref.casing)
            renderNavLineString(ref, map, false);
        else if (ref.geomType == NavGeomType::Text)
            renderNavText(ref, map, placedLabels);
    }
}

/**
 * @brief Render a NAV Text feature with collision detection.
 * @param ref FeatureRef structure.
 * @param map Target sprite.
 * @param placedLabels Persistent buffer for label collision detection.
 */
void Maps::renderNavText(const FeatureRef& ref, TFT_eSprite& map, std::vector<LabelRect, PsramAllocator<LabelRect>>& placedLabels)
{
    uint8_t* p = ref.ptr;
    int16_t tx, ty;
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
    float scale = (ref.width == 0) ? 0.8f : (ref.width == 1) ? 1.0f : 1.2f;
    map.setTextSize(scale);
    int tw = map.textWidth(textBuf);
    int th = map.fontHeight();
    int lx = px - tw / 2;
    int ly = py - th;
    const int PAD = 4;
    if (lx + tw < 0 || lx >= (int)tileWidth || ly + th < 0 || ly >= (int)tileHeight)
        return;
    bool collision = false;
    for (const auto& r : placedLabels)
    {
        if (lx - PAD < r.x + r.w && lx + tw + PAD > r.x && ly - PAD < r.y + r.h && ly + th + PAD > r.y)
        {
            collision = true;
            break;
        }
    }
    if (collision)
        return;
    map.setTextColor(ref.color);
    map.setTextDatum(lgfx::top_center);
    map.drawString(textBuf, px, ly);
    map.setTextDatum(lgfx::top_left);
    if (placedLabels.size() < placedLabels.capacity())
        placedLabels.push_back({(int16_t)lx, (int16_t)ly, (int16_t)tw, (int16_t)th});
}

/**
 * @brief Main entry point for NAV viewport rendering. Renders a full grid.
 * @param centerLat Center latitude.
 * @param centerLon Center longitude.
 * @param zoom Zoom level.
 * @param map Target sprite.
 * @return true if successful.
 */
bool Maps::renderNavViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite& map)
{
    const double latRad = (double)centerLat * M_PI / 180.0;
    const double n = pow(2.0, (double)zoom);
    const int centerTileIdxX = (int)floorf((float)((centerLon + 180.0) / 360.0 * n));
    const int centerTileIdxY = (int)floorf((float)((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n));
    const int8_t gridOffset = tilesGrid / 2;
    navTlTileX_ = (float)(centerTileIdxX - gridOffset);
    navTlTileY_ = (float)(centerTileIdxY - gridOffset);
    bool zoomChanged = (zoom != navLastZoom_);
    navLastZoom_ = zoom;
    if (mapSet.vectorMap)
    {
        viewportStartTime = esp_timer_get_time();
        totalLoadTime = 0; totalProcessTime = 0; totalDrawTime = 0;
        totalFeaturesDrawn = 0; cacheHits = 0; cacheMisses = 0;
    }
    if (xSemaphoreTake(mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        if (zoomChanged)
        {
            map.fillSprite(0xF7BE);
            redrawMap = true;
        }
        pendingTiles.clear();
        // Spiral order for 3x3 grid (queued in reverse for pop_back processing): 
        // 1. Corners, 2. Sides, 3. Center
        static const int8_t spiralOrder[9][2] = {{0,0}, {2,0}, {0,2}, {2,2}, {0,1}, {1,0}, {2,1}, {1,2}, {1,1}};
        for (int i = 0; i < 9; i++)
        {
            int dx = spiralOrder[i][0], dy = spiralOrder[i][1];
            pendingTiles.push_back({(uint32_t)(centerTileIdxX - gridOffset + dx), (uint32_t)(centerTileIdxY - gridOffset + dy), (int16_t)(dx * 256), (int16_t)(dy * 256), TILE_NAV});
        }
        xSemaphoreGive(mapMutex);
    }
    return true;
}
/**
 * @brief Render a single NAV tile at a specific sprite position.
 * @details Loads the binary NAV data from SD and renders geometries according to priority layers.
 * @param tileX Tile X coordinate.
 * @param tileY Tile Y coordinate.
 * @param zoom Zoom level.
 * @param screenX X position in the target sprite.
 * @param screenY Y position in the target sprite.
 * @param map Target sprite.
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
        if (!NavReader::openPack(zoom)) return;
        uint32_t offset, size;
        if (!NavReader::findTileInPack(tileX, tileY, offset, size)) return;
        data = (uint8_t*)heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM);
        if (!data)
        {
            for (int i = (int)navDataCache.size()-1; i >= 0; i--)
                if (!navDataCache[i].isPinned) { heap_caps_free(navDataCache[i].data); navDataCache.erase(navDataCache.begin() + i); }
            data = (uint8_t*)heap_caps_aligned_alloc(512, size, MALLOC_CAP_SPIRAM);
            if (!data) return;
        }
        storage.seek(NavReader::packFile, offset, SEEK_SET);
        if (storage.read(NavReader::packFile, data, size) != size) { heap_caps_free(data); return; }
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
    uint64_t tLoadEnd = 0;
    if (mapSet.vectorMap)
    {
        tLoadEnd = esp_timer_get_time();
        totalLoadTime += (tLoadEnd - tStart);
    }
    if (dataSize < 22) return;
    uint16_t feature_count;
    memcpy(&feature_count, data + 4, 2);
    uint8_t* p = data + 22;
    for (uint16_t i = 0; i < feature_count; i++)
    {
        if (p + 13 > data + dataSize) break;
        uint8_t geomType = p[0], zp = p[3], wp = p[4], bx1 = p[5], by1 = p[6], bx2 = p[7], by2 = p[8];
        uint16_t colorRgb565, cc, ps;
        memcpy(&colorRgb565, p + 1, 2);
        memcpy(&cc, p + 9, 2);
        memcpy(&ps, p + 11, 2);
        if (p + 13 + ps > data + dataSize) break;
        if ((zp >> 4) <= zoom)
        {
            if (screenX + bx2 < 0 || screenX + bx1 > (int)tileWidth || screenY + by2 < 0 || screenY + by1 > (int)tileHeight)
            {
                p += 13 + ps;
                continue;
            }
            // Phase 13: Semantic size filter based on zoom
            int16_t dimX = bx2 - bx1, dimY = by2 - by1;
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
                if (priority < 16) layers[priority].push_back(poolIdx);
            }
        }
        p += 13 + ps;
    }
    totalProcessTime += (mapSet.vectorMap) ? (esp_timer_get_time() - tLoadEnd) : 0;
}