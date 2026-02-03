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
               navLastLat_(0), navLastLon_(0), navLastZoom_(0), navNeedsRender_(true),
               navTlTileX_(-1), navTlTileY_(-1)
{
    projBuf16X.reserve(1024);
    projBuf16Y.reserve(1024);
    projBuf32X.reserve(1024);
    projBuf32Y.reserve(1024);
    edgePool.reserve(1024);
    edgeBuckets.reserve(tileHeight);

    mapMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(mapRenderTask, "MapRenderTask", 8192, this, 1, &mapRenderTaskHandle, 0);
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
    Maps::zoomLevel = zoom;
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
        
        // Handle immediate track redraw even if map hasn't moved
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

        if (!zoomChanged && !tileChanged && !navNeedsRender_ && pendingTiles.empty())
            return;

        bool needsFullRender = zoomChanged || navNeedsRender_ || (navTlTileX_ == -1);

        if (!needsFullRender && !tileChanged)
            return;

        if (!needsFullRender)
        {
            int32_t dx = currentTlX - (int32_t)navTlTileX_;
            int32_t dy = currentTlY - (int32_t)navTlTileY_;
            
            if (abs(dx) > 1 || abs(dy) > 1)
                needsFullRender = true;
            else if (dx != 0 || dy != 0)
            {
                if (xSemaphoreTake(mapMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    // Visual shift and coordinate update MUST happen together under Mutex
                    mapTempSprite.scroll(-(int16_t)dx * 256, -(int16_t)dy * 256);
                    navTlTileX_ = (float)currentTlX;
                    navTlTileY_ = (float)currentTlY;
                    
                    pendingTiles.clear();
                    if (dx != 0)
                    {
                        int32_t targetX = (dx > 0) ? (tilesGrid - 1) : 0;
                        mapTempSprite.fillRect((int16_t)targetX * 256, 0, 256, tileHeight, TFT_WHITE);
                        for (int y = 0; y < tilesGrid; y++)
                            pendingTiles.push_back({(uint32_t)(currentTlX + targetX), (uint32_t)(currentTlY + y), (int16_t)(targetX * 256), (int16_t)(y * 256), TILE_NAV});
                    }
                    if (dy != 0)
                    {
                        int32_t targetY = (dy > 0) ? (tilesGrid - 1) : 0;
                        mapTempSprite.fillRect(0, (int16_t)targetY * 256, tileWidth, 256, TFT_WHITE);
                        for (int x = 0; x < tilesGrid; x++)
                        {
                            if (dx != 0 && x == ((dx > 0) ? (tilesGrid - 1) : 0))
                                continue;
                            pendingTiles.push_back({(uint32_t)(currentTlX + x), (uint32_t)(currentTlY + targetY), (int16_t)(x * 256), (int16_t)(targetY * 256), TILE_NAV});
                        }
                    }
                    xSemaphoreGive(mapMutex);
                }

                navLastLat_ = lat;
                navLastLon_ = lon;
                navNeedsRender_ = false;
                navLastZoom_ = zoom;
                return;
            }
        }
        
        if (needsFullRender)
        {
            Maps::isMapFound = renderNavViewport(lat, lon, zoom, Maps::mapTempSprite);
            navLastLat_ = lat;
            navLastLon_ = lon;
            navLastZoom_ = zoom;
            navNeedsRender_ = false;
        }
        return;
    }

    Maps::currentMapTile = Maps::getMapTile(lon, lat, Maps::zoomLevel, 0, 0);
    if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 ||
        Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
        Maps::currentMapTile.tilex != Maps::oldMapTile.tilex ||
        Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
    {
        const int16_t size = Maps::mapTileSize;
        Maps::mapTempSprite.fillScreen(TFT_WHITE);
        Maps::isMapFound = Maps::mapTempSprite.drawPngFile(Maps::currentMapTile.file, size, size);
        Maps::oldMapTile = Maps::currentMapTile;
        if (!Maps::isMapFound)
        {
            Maps::mapTempSprite.fillScreen(TFT_BLACK);
            Maps::showNoMap(Maps::mapTempSprite);
        }
        else
        {
            Maps::totalBounds = Maps::getTileBounds(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);
            bool missingMap = false;
            for (int8_t y = -1; y <= 1; y++)
            {
                for (int8_t x = -1; x <= 1; x++)
                {
                    if (x == 0 && y == 0) continue;
                    const int16_t offsetX = (x + 1) * size;
                    const int16_t offsetY = (y + 1) * size;
                    Maps::roundMapTile = getMapTile(Maps::currentMapTile.lon, Maps::currentMapTile.lat, Maps::zoomLevel, x, y);
                    if (!Maps::mapTempSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY))
                    {
                        Maps::mapTempSprite.fillRect(offsetX, offsetY, size, size, TFT_BLACK);
                        Maps::mapTempSprite.drawPngFile(noMapFile, offsetX + size / 2 - 50, offsetY + size / 2 - 50);
                        missingMap = true;
                    }
                    else
                    {
                        const tileBounds currentBounds = Maps::getTileBounds(Maps::roundMapTile.tilex, Maps::roundMapTile.tiley, Maps::zoomLevel);
                        if (currentBounds.lat_min < Maps::totalBounds.lat_min) Maps::totalBounds.lat_min = currentBounds.lat_min;
                        if (currentBounds.lat_max > Maps::totalBounds.lat_max) Maps::totalBounds.lat_max = currentBounds.lat_max;
                        if (currentBounds.lon_min < Maps::totalBounds.lon_min) Maps::totalBounds.lon_min = currentBounds.lon_min;
                        if (currentBounds.lon_max > Maps::totalBounds.lon_max) Maps::totalBounds.lon_max = currentBounds.lon_max;
                    }
                }
            }
            if (!missingMap && Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
                Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &wptPosX, &wptPosY);
            else
            {
                Maps::wptPosX = -1;
                Maps::wptPosY = -1;
            }
            for (size_t i = 1; i < trackData.size(); ++i)
            {
                const auto &p1 = trackData[i - 1];
                const auto &p2 = trackData[i];
                if (p1.lon > Maps::totalBounds.lon_min && p1.lon < Maps::totalBounds.lon_max &&
                    p1.lat > Maps::totalBounds.lat_min && p1.lat < Maps::totalBounds.lat_max &&
                    p2.lon > Maps::totalBounds.lon_min && p2.lon < Maps::totalBounds.lon_max &&
                    p2.lat > Maps::totalBounds.lat_min && p2.lat < Maps::totalBounds.lat_max)
                {
                    uint16_t x1, y1, x2, y2;
                    Maps::coords2map(p1.lat, p1.lon, Maps::totalBounds, &x1, &y1);
                    Maps::coords2map(p2.lat, p2.lon, Maps::totalBounds, &x2, &y2);
                    Maps::mapTempSprite.drawWideLine(x1, y1, x2, y2, 2, TFT_BLUE);
                }
            }
            Maps::redrawMap = true;
        }
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
    while (1)
    {
        if (!instance->pendingTiles.empty())
        {
            if (xSemaphoreTake(instance->mapMutex, pdMS_TO_TICKS(200)) == pdTRUE)
            {
                if (!instance->pendingTiles.empty())
                {
                    PendingTile t = instance->pendingTiles.back();
                    instance->pendingTiles.pop_back();
                    if (t.type == TILE_NAV)
                        instance->renderNavTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                    else if (t.type == TILE_PNG)
                        instance->renderPngTile(t.x, t.y, instance->zoomLevel, t.screenX, t.screenY, instance->mapTempSprite);
                    
                    if (instance->pendingTiles.empty())
                    {
                        instance->drawTrack(instance->mapTempSprite);
                        instance->redrawMap = true;
                    }
                }
                xSemaphoreGive(instance->mapMutex);
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
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
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
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);

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
    Maps::currentMapTile.tilex = Maps::lon2tilex(lon, Maps::currentMapTile.zoom);
    Maps::currentMapTile.tiley = Maps::lat2tiley(lat, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = lat;
    Maps::currentMapTile.lon = lon;
    Maps::offsetX = 0;
    Maps::offsetY = 0;
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
void Maps::fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset, uint16_t ringCount, uint16_t* ringEnds)
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
    uint16_t* ends = (ringEnds == nullptr) ? defaultEnds : ringEnds;
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
        if (activeHead == -1) continue;
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
 * @brief Render a NAV LineString feature
 * @param feature NavFeature object.
 * @param map Target sprite.
 */
void Maps::renderNavLineString(const NavFeature& feature, TFT_eSprite& map)
{
    if (feature.coordCount < 2) return;
    const size_t numCoords = feature.coordCount;
    if (projBuf16X.capacity() < numCoords) projBuf16X.reserve(numCoords * 1.5);
    if (projBuf16Y.capacity() < numCoords) projBuf16Y.reserve(numCoords * 1.5);
    projBuf16X.resize(numCoords);
    projBuf16Y.resize(numCoords);
    int16_t* pxArr = projBuf16X.data();
    int16_t* pyArr = projBuf16Y.data();
    int16_t minPx = INT16_MAX, maxPx = INT16_MIN;
    int16_t minPy = INT16_MAX, maxPy = INT16_MIN;
    size_t validPoints = 0;
    int16_t lastPx = -32768, lastPy = -32768;
    for (size_t i = 0; i < numCoords; i++)
    {
        int16_t px, py;
        navCoordToPixel(feature, feature.coords[i], px, py);
        if (validPoints > 0 && px == lastPx && py == lastPy) continue;
        pxArr[validPoints] = px;
        pyArr[validPoints] = py;
        if (px < minPx) minPx = px;
        if (px > maxPx) maxPx = px;
        if (py < minPy) minPy = py;
        if (py > maxPy) maxPy = py;
        lastPx = px;
        lastPy = py;
        validPoints++;
    }
    if (validPoints < 2 || maxPx < 0 || minPx >= tileWidth || maxPy < 0 || minPy >= tileHeight) return;
    uint16_t color = feature.properties.colorRgb565;
    uint8_t width = feature.properties.width > 0 ? feature.properties.width : 1;
    for (size_t i = 1; i < validPoints; i++)
    {
        if (width == 1)
            map.drawLine(pxArr[i - 1], pyArr[i - 1], pxArr[i], pyArr[i], color);
        else
            map.drawWideLine(pxArr[i - 1], pyArr[i - 1], pxArr[i], pyArr[i], width, color);
    }
}

/**
 * @brief Render a NAV Polygon feature
 * @param feature NavFeature object.
 * @param map Target sprite.
 */
void Maps::renderNavPolygon(const NavFeature& feature, TFT_eSprite& map)
{
    if (feature.coordCount < 3) return;
    size_t numPoints = feature.coordCount;
    if (projBuf32X.capacity() < numPoints) projBuf32X.reserve(numPoints * 1.5);
    if (projBuf32Y.capacity() < numPoints) projBuf32Y.reserve(numPoints * 1.5);
    projBuf32X.resize(numPoints);
    projBuf32Y.resize(numPoints);
    int* px = projBuf32X.data();
    int* py = projBuf32Y.data();
    int minPx = INT_MAX, maxPx = INT_MIN;
    int minPy = INT_MAX, maxPy = INT_MIN;
    for (size_t i = 0; i < numPoints; i++)
    {
        int16_t x, y;
        navCoordToPixel(feature, feature.coords[i], x, y);
        px[i] = x;
        py[i] = y;
        if (x < minPx) minPx = x;
        if (x > maxPx) maxPx = x;
        if (y < minPy) minPy = y;
        if (y > maxPy) maxPy = y;
    }
    if (maxPx < 0 || minPx >= tileWidth || maxPy < 0 || minPy >= tileHeight) return;
    uint16_t fillColor = feature.properties.colorRgb565;
    uint16_t borderColor = darkenRGB565(fillColor, 0.15f);
    if (fillPolygons)
        fillPolygonGeneral(map, px, py, numPoints, fillColor, 0, 0, feature.ringCount, feature.ringEnds);
    int extEnd = (feature.ringCount > 0) ? feature.ringEnds[0] : numPoints;
    if (extEnd >= 3)
    {
        for (int i = 0; i < extEnd; i++)
        {
            int next = (i + 1 == extEnd) ? 0 : i + 1;
            map.drawLine(px[i], py[i], px[next], py[next], borderColor);
        }
    }
}

/**
 * @brief Render a NAV Point feature
 * @param feature NavFeature object.
 * @param map Target sprite.
 */
void Maps::renderNavPoint(const NavFeature& feature, TFT_eSprite& map)
{
    if (feature.coordCount == 0) return;
    int16_t px, py;
    navCoordToPixel(feature, feature.coords[0], px, py);
    if (px >= 0 && px < tileWidth && py >= 0 && py < tileHeight)
        map.fillCircle(px, py, 3, feature.properties.colorRgb565);
}

/**
 * @brief Render a single NAV feature
 * @param feature NavFeature to render.
 * @param viewport Current viewport bbox.
 * @param map Target sprite.
 */
void Maps::renderNavFeature(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map)
{
    switch (feature.geomType)
    {
        case NavGeomType::LineString: renderNavLineString(feature, map); break;
        case NavGeomType::Polygon: renderNavPolygon(feature, map); break;
        case NavGeomType::Point: renderNavPoint(feature, map); break;
        default: break;
    }
}

/**
 * @brief Convert internal NAV coordinate to pixels
 * @param feature Current NavFeature context.
 * @param coord NavCoord (0-4096).
 * @param px Output X pixel.
 * @param py Output Y pixel.
 */
void Maps::navCoordToPixel(const NavFeature& feature, const NavCoord& coord, int16_t& px, int16_t& py)
{
    px = feature.tilePixelOffsetX + (coord.x >> 4);
    py = feature.tilePixelOffsetY + (coord.y >> 4);
}

/**
 * @brief Main entry point for NAV viewport rendering. Renders a full grid.
 * @details Calculates the top-left tile based on current GPS coordinates and prepares 
 *          a grid of tiles (dynamic size based on tilesGrid) for asynchronous rendering.
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
    navLastZoom_ = zoom;
    if (xSemaphoreTake(mapMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        map.fillSprite(TFT_WHITE);
        pendingTiles.clear();
        for (int dy = 0; dy < tilesGrid; dy++)
        {
            for (int dx = 0; dx < tilesGrid; dx++)
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
    char tilePath[128];
    snprintf(tilePath, sizeof(tilePath), mapVectorFolder, zoom, tileX, tileY);
    std::vector<NavFeature> tileFeatures;
    uint8_t* tileBuffer = nullptr;
    size_t tileBufferSize = 0;
    if (NavReader::readAllFeaturesMemory(tilePath, tileFeatures, zoom, tileBuffer, tileBufferSize) == 0)
    {
        if (tileBuffer)
            heap_caps_free(tileBuffer);
        return;
    }
    std::vector<NavFeature> layers[16];
    for (auto& feature : tileFeatures)
    {
        feature.tilePixelOffsetX = screenX;
        feature.tilePixelOffsetY = screenY;
        uint8_t priority = feature.properties.getPriority();
        if (priority < 16)
            layers[priority].push_back(feature);
    }
    map.startWrite();
    for (int p = 0; p < 16; p++)
    {
        if (layers[p].empty())
            continue;
        for (const auto& feature : layers[p])
        {
            const int16_t minX = feature.tilePixelOffsetX + feature.objBbox.x1;
            const int16_t minY = feature.tilePixelOffsetY + feature.objBbox.y1;
            const int16_t maxX = feature.tilePixelOffsetX + feature.objBbox.x2;
            const int16_t maxY = feature.tilePixelOffsetY + feature.objBbox.y2;
            if (minX > (int16_t)tileWidth || maxX < 0 || minY > (int16_t)tileHeight || maxY < 0)
                continue;
            map.setClipRect(screenX, screenY, 256, 256);
            renderNavFeature(feature, {}, map);
        }
        taskYIELD();
    }
    map.clearClipRect();
    map.endWrite();
    if (tileBuffer)
        heap_caps_free(tileBuffer);
}