/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.5
 * @date 2025-11
 */

#include "maps.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include <cmath> 

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData;
const char* TAG PROGMEM = "Maps";

uint16_t Maps::currentDrawColor = TFT_WHITE;
uint8_t Maps::PALETTE[256] = {0};
uint32_t Maps::PALETTE_SIZE = 0;

// Static variables initialization
std::vector<Maps::CachedTile> Maps::tileCache;
size_t Maps::maxCachedTiles = 0;
uint32_t Maps::cacheAccessCounter = 0;

std::vector<Maps::UnifiedPoolEntry> Maps::unifiedPool;
SemaphoreHandle_t Maps::unifiedPoolMutex = nullptr;
size_t Maps::maxUnifiedPoolEntries = 0;
uint32_t Maps::unifiedPoolHitCount = 0;
uint32_t Maps::unifiedPoolMissCount = 0;

uint32_t Maps::totalMemoryAllocations = 0;
uint32_t Maps::totalMemoryDeallocations = 0;
uint32_t Maps::peakMemoryUsage = 0;
uint32_t Maps::currentMemoryUsage = 0;
uint32_t Maps::poolEfficiencyScore = 0;
uint32_t Maps::lastStatsUpdate = 0;

bool Maps::polygonCullingEnabled = true;
bool Maps::optimizedScanlineEnabled = true;
uint32_t Maps::polygonRenderCount = 0;
uint32_t Maps::polygonCulledCount = 0;
uint32_t Maps::polygonOptimizedCount = 0;

Maps::RenderBatch* Maps::activeBatch = nullptr;
size_t Maps::maxBatchSize = 0;
uint32_t Maps::batchRenderCount = 0;
uint32_t Maps::batchOptimizationCount = 0;
uint32_t Maps::batchFlushCount = 0;

Maps::Maps() : fillPolygons(true) 
{
    ESP_LOGI(TAG, "Maps constructor completed");
}

// --- Helper Functions ---
uint16_t Maps::lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize) {
    return static_cast<uint16_t>(((f_lon + 180.0f) / 360.0f * (1 << zoom) * tileSize)) % tileSize;
}
uint16_t Maps::lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize) {
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
    float scale = (1 << zoom) * tileSize;
    return static_cast<uint16_t>(((1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * scale)) % tileSize;
}
uint32_t Maps::lon2tilex(float f_lon, uint8_t zoom) {
    float rawTile = (f_lon + 180.0f) / 360.0f * (1 << zoom); rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}
uint32_t Maps::lat2tiley(float f_lat, uint8_t zoom) {
    float lat_rad = f_lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
    float rawTile = (1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * (1 << zoom); rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}
float Maps::tilex2lon(uint32_t tileX, uint8_t zoom) {
    return static_cast<float>(tileX) * 360.0f / (1 << zoom) - 180.0f;
}
float Maps::tiley2lat(uint32_t tileY, uint8_t zoom) {
    float scale = static_cast<float>(1 << zoom);
    float n = static_cast<float>(M_PI) * (1.0f - 2.0f * static_cast<float>(tileY) / scale);
    return 180.0f / static_cast<float>(M_PI) * atanf(sinhf(n));
}

Maps::MapTile Maps::getMapTile(float lon, float lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY)
{
    MapTile data;
    data.tilex = Maps::lon2tilex(lon, zoomLevel) + offsetX;
    data.tiley = Maps::lat2tiley(lat, zoomLevel) + offsetY;
    data.zoom = zoomLevel;
    data.lat = lat; 
    data.lon = lon;
    
    if (mapSet.vectorMap)
        snprintf(data.file, sizeof(data.file), mapVectorFolder, zoomLevel, data.tilex, data.tiley);
    else
        snprintf(data.file, sizeof(data.file), mapRenderFolder, zoomLevel, data.tilex, data.tiley);

    return data;
}

Maps::tileBounds Maps::getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom)
{
    tileBounds bounds;
    bounds.lon_min = Maps::tilex2lon(tileX, zoom);
    bounds.lat_min = Maps::tiley2lat(tileY + 1, zoom);
    bounds.lon_max = Maps::tilex2lon(tileX + 1, zoom);
    bounds.lat_max = Maps::tiley2lat(tileY, zoom);
    return bounds;
}

bool Maps::isCoordInBounds(float lat, float lon, tileBounds bound)
{
    return (lat >= bound.lat_min && lat <= bound.lat_max &&
            lon >= bound.lon_min && lon <= bound.lon_max);
}

Maps::ScreenCoord Maps::coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize)
{
    ScreenCoord data;
    data.posX = Maps::lon2posx(lon, zoomLevel, tileSize);
    data.posY = Maps::lat2posy(lat, zoomLevel, tileSize);
    return data;
}

void Maps::coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY)
{
    float lon_ratio = (lon - bound.lon_min) / (bound.lon_max - bound.lon_min);
    float lat_ratio = (bound.lat_max - lat) / (bound.lat_max - bound.lat_min);

    *pixelX = (uint16_t)(lon_ratio * Maps::tileWidth);
    *pixelY = (uint16_t)(lat_ratio * Maps::tileHeight);
}

void Maps::showNoMap(TFT_eSprite &map)
{
    map.drawPngFile(noMapFile, (Maps::mapScrWidth / 2) - 50, (Maps::mapScrHeight / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (Maps::mapScrWidth / 2), (Maps::mapScrHeight >> 1) + 65, &fonts::DejaVu18);
}

void Maps::initMap(uint16_t mapHeight, uint16_t mapWidth)
{
    Maps::mapScrHeight = mapHeight;
    Maps::mapScrWidth = mapWidth;
    Maps::mapTempSprite.deleteSprite();
    Maps::mapTempSprite.createSprite(tileHeight, tileWidth);
    Maps::oldMapTile = {}; 
    Maps::currentMapTile = {}; 
    Maps::roundMapTile = {}; 
    Maps::navArrowPosition = {0, 0};
    Maps::totalBounds = {90.0, -90.0, 180.0, -180.0};
    initTileCache();
    polygonCullingEnabled = true;
    optimizedScanlineEnabled = false;
    polygonRenderCount = 0;
    polygonCulledCount = 0;
    polygonOptimizedCount = 0;
    initBatchRendering();
}

void Maps::deleteMapScrSprites()
{
    Maps::mapSprite.deleteSprite();
    clearTileCache();
}

void Maps::createMapScrSprites()
{
    Maps::mapBuffer = Maps::mapSprite.createSprite(Maps::mapScrWidth, Maps::mapScrHeight);
}

// --- generateMap (Updated) ---
void Maps::generateMap(uint8_t zoom)
{
    if (Maps::zoomLevel != zoom && Maps::zoomLevel != 0)
    {
        ESP_LOGI(TAG, "Zoom level changed from %d to %d - clearing cache", Maps::zoomLevel, zoom);
        clearTileCache();
    }
    
    Maps::zoomLevel = zoom;
    
    bool foundRoundMap = false;
    bool missingMap = false;

    const float lat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
    const float lon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

    Maps::currentMapTile = Maps::getMapTile(lon, lat, Maps::zoomLevel, 0, 0);

    if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 ||
        Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
        Maps::currentMapTile.tilex != Maps::oldMapTile.tilex ||
        Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
    {
        const int16_t size = Maps::mapTileSize;
        Maps::mapTempSprite.fillScreen(TFT_WHITE);

        if (mapSet.vectorMap)
            Maps::isMapFound = renderTile(Maps::currentMapTile.file, size, size,Maps::mapTempSprite);
        else
            Maps::isMapFound = Maps::mapTempSprite.drawPngFile(Maps::currentMapTile.file, size, size);
        
        Maps::oldMapTile = Maps::currentMapTile;
        strcpy(Maps::oldMapTile.file, Maps::currentMapTile.file);

        if (!Maps::isMapFound)
        {
            ESP_LOGI(TAG, "No Map Found!");
            Maps::isMapFound = false;
            Maps::mapTempSprite.fillScreen(TFT_BLACK);
            Maps::showNoMap(Maps::mapTempSprite);
        }
        else
        {
            Maps::totalBounds = Maps::getTileBounds(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);
            const int8_t startX = -1;
            const int8_t startY = -1;

            for (int8_t y = startY; y <= startY + 2; y++)
            {
                const int16_t offsetY = (y - startY) * size;
                for (int8_t x = startX; x <= startX + 2; x++)
                {
                    if (x == 0 && y == 0) continue;
                    const int16_t offsetX = (x - startX) * size;
                    Maps::roundMapTile = getMapTile(Maps::currentMapTile.lon, Maps::currentMapTile.lat, Maps::zoomLevel, x, y);
 
                    if (mapSet.vectorMap)
                        foundRoundMap = renderTile(Maps::roundMapTile.file, offsetX, offsetY,Maps::mapTempSprite);
                    else
                        foundRoundMap = Maps::mapTempSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);

                    if (!foundRoundMap)
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

            if (!missingMap)
            {
                if (Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
                    Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &wptPosX, &wptPosY);
            }
            else
            {
                Maps::wptPosX = -1;
                Maps::wptPosY = -1;
            }

            Maps::redrawMap = true;
            
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
                    Maps::mapTempSprite.drawWideLine(x1, y1, x2, y2, 2.0f, TFT_BLUE);
                }
            }
            // THIS CALL caused the linker error because definition was missing
            prefetchAdjacentTiles(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);
        }
    }
}

void Maps::displayMap()
{
    if (!Maps::isMapFound)
    {
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
        return;
    }

    uint16_t mapHeading = 0;
#ifdef ENABLE_COMPASS
    mapHeading = mapSet.mapRotationComp ? compass.getHeading() : gps.gpsData.heading;
#else
    mapHeading = gps.gpsData.heading;
#endif

    Maps::mapTempSprite.pushImage(Maps::wptPosX - 8, Maps::wptPosY - 8, 16, 16, (uint16_t *)waypoint, TFT_BLACK);

    if (Maps::followGps)
    {
        const float lat = gps.gpsData.latitude;
        const float lon = gps.gpsData.longitude;
        Maps::navArrowPosition = Maps::coord2ScreenPos(lon, lat, Maps::zoomLevel, Maps::mapTileSize);
        Maps::mapTempSprite.setPivot(Maps::mapTileSize + Maps::navArrowPosition.posX,
                                     Maps::mapTileSize + Maps::navArrowPosition.posY);
        Maps::mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
    }
    else
    {
        const int16_t pivotX = Maps::tileWidth / 2 + Maps::offsetX;
        const int16_t pivotY = Maps::tileHeight / 2 + Maps::offsetY;
        Maps::mapTempSprite.setPivot(pivotX, pivotY);
        Maps::mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);
    }
}

void Maps::setWaypoint(float wptLat, float wptLon) {
    Maps::destLat = wptLat;
    Maps::destLon = wptLon;
}

void Maps::updateMap() {
    Maps::oldMapTile = {};
}

void Maps::panMap(int8_t dx, int8_t dy) {
    Maps::currentMapTile.tilex += dx;
    Maps::currentMapTile.tiley += dy;
    Maps::currentMapTile.lon = Maps::tilex2lon(Maps::currentMapTile.tilex, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = Maps::tiley2lat(Maps::currentMapTile.tiley, Maps::currentMapTile.zoom);
}

void Maps::centerOnGps(float lat, float lon) {
    Maps::followGps = true;
    Maps::currentMapTile.tilex = Maps::lon2tilex(lon, Maps::currentMapTile.zoom);
    Maps::currentMapTile.tiley = Maps::lat2tiley(lat, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = lat;
    Maps::currentMapTile.lon = lon;
}

void Maps::scrollMap(int16_t dx, int16_t dy)
{
    const float inertia = 0.5f;
    const float friction = 0.95f;
    const float maxSpeed = 10.0f;
    static float speedX = 0.0f, speedY = 0.0f;

    speedX = (speedX + dx) * inertia * friction;
    speedY = (speedY + dy) * inertia * friction;

    const float absSpeedX = fabsf(speedX);
    const float absSpeedY = fabsf(speedY);

    if (absSpeedX > maxSpeed) speedX = (speedX > 0) ? maxSpeed : -maxSpeed;
    if (absSpeedY > maxSpeed) speedY = (speedY > 0) ? maxSpeed : -maxSpeed;

    Maps::offsetX += (int16_t)speedX;
    Maps::offsetY += (int16_t)speedY;

    Maps::scrollUpdated = false;
    Maps::followGps = false;

    const int16_t threshold = Maps::scrollThreshold;
    const int16_t tileSize = Maps::mapTileSize;

    if (Maps::offsetX <= -threshold) { Maps::tileX--; Maps::offsetX += tileSize; Maps::scrollUpdated = true; }
    else if (Maps::offsetX >= threshold) { Maps::tileX++; Maps::offsetX -= tileSize; Maps::scrollUpdated = true; }

    if (Maps::offsetY <= -threshold) { Maps::tileY--; Maps::offsetY += tileSize; Maps::scrollUpdated = true; }
    else if (Maps::offsetY >= threshold) { Maps::tileY++; Maps::offsetY -= tileSize; Maps::scrollUpdated = true; }

    if (Maps::scrollUpdated)
    {
        const int8_t deltaTileX = Maps::tileX - Maps::lastTileX;
        const int8_t deltaTileY = Maps::tileY - Maps::lastTileY;
        Maps::panMap(deltaTileX, deltaTileY);
        Maps::preloadTiles(deltaTileX, deltaTileY);
        Maps::lastTileX = Maps::tileX;
        Maps::lastTileY = Maps::tileY;
    }
}

void Maps::preloadTiles(int8_t dirX, int8_t dirY)
{
    const int16_t tileSize = mapTileSize;
    const int16_t preloadWidth  = (dirX != 0) ? tileSize : tileSize * 2;
    const int16_t preloadHeight = (dirY != 0) ? tileSize : tileSize * 2;

    TFT_eSprite preloadSprite = TFT_eSprite(&tft);
    preloadSprite.createSprite(preloadWidth, preloadHeight);
    
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
        
        if (mapSet.vectorMap) {
            foundTile = getCachedTile(Maps::roundMapTile.file, preloadSprite, offsetX, offsetY);
        }
        
        if (!foundTile) {
            if (mapSet.vectorMap) {
                TFT_eSprite tempSprite = TFT_eSprite(&tft);
                tempSprite.createSprite(tileSize, tileSize);
                foundTile = renderTile(Maps::roundMapTile.file, 0, 0, tempSprite);
                if (foundTile) preloadSprite.pushImage(offsetX, offsetY, tileSize, tileSize, tempSprite.frameBuffer(0));
                tempSprite.deleteSprite();
            } else 
                foundTile = preloadSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);
        }
        if (!foundTile) preloadSprite.fillRect(offsetX, offsetY, tileSize, tileSize, TFT_LIGHTGREY);
    }

    if (dirX != 0) {
        mapTempSprite.scroll(dirX * tileSize, 0);
        const int16_t pushX = (dirX > 0) ? tileSize * 2 : 0;
        mapTempSprite.pushImage(pushX, 0, preloadWidth, preloadHeight, preloadSprite.frameBuffer(0));
    } else if (dirY != 0) {
        mapTempSprite.scroll(0, dirY * tileSize);
        const int16_t pushY = (dirY > 0) ? tileSize * 2 : 0;
        mapTempSprite.pushImage(0, pushY, preloadWidth, preloadHeight, preloadSprite.frameBuffer(0));
    }
    preloadSprite.deleteSprite();
}

/**
 * @brief Implementación de la función que faltaba, responsable del error de enlazado.
 */
void Maps::prefetchAdjacentTiles(int16_t centerX, int16_t centerY, uint8_t zoom)
{
    // Prefetch 3x3 grid around current tile
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0) continue; // Skip center tile
            
            int16_t tileX = centerX + dx;
            int16_t tileY = centerY + dy;
            
            // Calculate tile coordinates
            float tileLon = (tileX / (1 << zoom)) * 360.0f - 180.0f;
            float tileLat = 90.0f - (tileY / (1 << zoom)) * 180.0f;
            
            MapTile prefetchTile = getMapTile(tileLon, tileLat, zoom, tileX, tileY);
            
            // Check if tile exists and not already in cache
            FILE* testFile = fopen(prefetchTile.file, "rb");
            if (testFile)
            {
                fclose(testFile);
                
                // Try to load into cache if not already there
                TFT_eSprite tempSprite = TFT_eSprite(&tft);
                tempSprite.createSprite(tileWidth, tileHeight);
                
                if (!getCachedTile(prefetchTile.file, tempSprite, 0, 0))
                {
                    // Render tile to cache it
                    renderTile(prefetchTile.file, 0, 0, tempSprite);
                }
                
                tempSprite.deleteSprite();
            }
        }
    }
}

// --- Helper & Utility Methods ---

bool Maps::loadPalette(const char* palettePath)
{
    FILE* f = fopen(palettePath, "rb");
    if (!f) return false;
    
    uint32_t numColors;
    if (fread(&numColors, 4, 1, f) != 1) { fclose(f); return false; }
    
    uint8_t rgb888[3];
    PALETTE_SIZE = 0;
    
    for (uint32_t i = 0; i < numColors && i < 256; i++) {
        if (fread(rgb888, 3, 1, f) == 1) {
            uint8_t r332 = rgb888[0] & 0xE0;
            uint8_t g332 = (rgb888[1] & 0xE0) >> 3;
            uint8_t b332 = rgb888[2] >> 6;
            PALETTE[i] = r332 | g332 | b332;
            PALETTE_SIZE++;
        }
    }
    fclose(f);
    ESP_LOGI(TAG, "Loaded palette: %u colors", PALETTE_SIZE);
    return PALETTE_SIZE > 0;
}

uint8_t Maps::paletteToRGB332(const uint32_t idx) {
    if (idx < PALETTE_SIZE) return PALETTE[idx];
    return 0xFF;
}

uint8_t Maps::darkenRGB332(const uint8_t color, const float amount = 0.4f) {
    uint8_t r = (color & 0xE0) >> 5;
    uint8_t g = (color & 0x1C) >> 2;
    uint8_t b = (color & 0x03);
    r = static_cast<uint8_t>(r * (1.0f - amount));
    g = static_cast<uint8_t>(g * (1.0f - amount));
    b = static_cast<uint8_t>(b * (1.0f - amount));
    return ((r << 5) | (g << 2) | b);
}

uint16_t Maps::RGB332ToRGB565(const uint8_t color) {
    uint8_t r = (color & 0xE0);
    uint8_t g = (color & 0x1C) << 3;
    uint8_t b = (color & 0x03) << 6;
    uint16_t r565 = (r >> 3) & 0x1F;
    uint16_t g565 = (g >> 2) & 0x3F; 
    uint16_t b565 = (b >> 3) & 0x1F; 
    return (r565 << 11) | (g565 << 5) | b565;
}

uint32_t Maps::readVarint(const uint8_t* data, size_t& offset, const size_t dataSize) {
    uint32_t value = 0;
    uint8_t shift = 0;
    while (offset < dataSize && shift < 32) {
        uint8_t byte = data[offset++];
        value |= ((uint32_t)(byte & 0x7F)) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    if (offset > dataSize) { offset = dataSize; return 0; }
    return value;
}

int32_t Maps::readZigzag(const uint8_t* data, size_t& offset, const size_t dataSize) {
    if (offset >= dataSize) return 0;
    const uint32_t encoded = Maps::readVarint(data, offset, dataSize);
    return static_cast<int32_t>((encoded >> 1) ^ (-(int32_t)(encoded & 1)));
}

int Maps::uint16ToPixel(const int32_t val) {
    int p = static_cast<int>((val * TILE_SIZE_PLUS_ONE) / 65536);
    if (p < 0) p = 0;
    if (p > TILE_SIZE) p = TILE_SIZE;
    return p;
}

bool Maps::isPointOnMargin(const int px, const int py) {
    return (px <= MARGIN_PIXELS || px >= TILE_SIZE - MARGIN_PIXELS || py <= MARGIN_PIXELS || py >= TILE_SIZE - MARGIN_PIXELS);
}

bool Maps::isNear(int val, int target, int tol = 2) {
    return abs(val - target) <= tol;
}

bool Maps::shouldDrawLine(const int px1, const int py1, const int px2, const int py2) {
    if ((isNear(px1, 0) && isNear(px2, TILE_SIZE)) || (isNear(px1, TILE_SIZE) && isNear(px2, 0))) {
        if ((isNear(py1, 0) && isNear(py2, TILE_SIZE)) || (isNear(py1, TILE_SIZE) && isNear(py2, 0))) return false;
        if (isNear(py1, py2)) return false; 
    }
    if ((isNear(py1, 0) && isNear(py2, TILE_SIZE)) || (isNear(py1, TILE_SIZE) && isNear(py2, 0))) {
        if (isNear(px1, px2)) return false; 
    }
    int dx = px2 - px1; int dy = py2 - py1; int len2 = dx*dx + dy*dy;
    if (len2 > (TILE_SIZE * TILE_SIZE * 3)) return false;
    if (isPointOnMargin(px1, py1) && isPointOnMargin(px2, py2)) return false;
    if ((px1 == px2) && (px1 <= MARGIN_PIXELS || px1 >= TILE_SIZE - MARGIN_PIXELS)) return false;
    if ((py1 == py2) && (py1 <= MARGIN_PIXELS || py1 >= TILE_SIZE - MARGIN_PIXELS)) return false;
    return true;
}

void Maps::fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset) {
    static bool unifiedPoolLogged = false;
    int miny = py[0], maxy = py[0];
    for (int i = 1; i < numPoints; ++i) {
        if (py[i] < miny) miny = py[i];
        if (py[i] > maxy) maxy = py[i];
    }
    int *xints = nullptr;
    MemoryGuard<int> xintsGuard(numPoints, 6);
    xints = xintsGuard.get();
    if (!xints) return;
    if (!unifiedPoolLogged) unifiedPoolLogged = true;
    unifiedPoolHitCount++;

    for (int y = miny; y <= maxy; ++y) {
        int nodes = 0;
        for (int i = 0, j = numPoints - 1; i < numPoints; j = i++) {
            if ((py[i] < y && py[j] >= y) || (py[j] < y && py[i] >= y))
                xints[nodes++] = static_cast<int>(px[i] + (y - py[i]) * (px[j] - px[i]) / (py[j] - py[i]));
        }
        if (nodes > 1) {
            std::sort(xints, xints + nodes);
            for (int i = 0; i < nodes; i += 2) {
                if (i + 1 < nodes) {
                    int x0 = xints[i] + xOffset;
                    int x1 = xints[i + 1] + xOffset;
                    int yy = y + yOffset;
                    if (yy >= 0 && yy <= TILE_SIZE + yOffset) {
                        if (x0 < 0) x0 = 0;
                        if (x1 > TILE_SIZE + xOffset) x1 = TILE_SIZE + xOffset;
                        map.drawLine(x0, yy, x1, yy, color);
                    }
                }
            }
        }
    }
}

void Maps::drawPolygonBorder(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t borderColor, const uint16_t fillColor, const int xOffset, const int yOffset) {
    if (numPoints < 2) return; 
    for (uint32_t i = 0; i < numPoints - 1; ++i) {
        const bool marginA = isPointOnMargin(px[i], py[i]);
        const bool marginB = isPointOnMargin(px[i+1], py[i+1]);
        const uint16_t color = (marginA && marginB) ? fillColor : borderColor;
        const int x0 = px[i] + xOffset;
        const int y0 = py[i] + yOffset;
        const int x1 = px[i+1] + xOffset;
        const int y1 = py[i+1] + yOffset;
        if (x0 >= 0 && x0 <= TILE_SIZE + xOffset && y0 >= 0 && y0 <= TILE_SIZE + yOffset &&
            x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset) {
            if (!(marginA && marginB && !fillPolygons)) map.drawLine(x0, y0, x1, y1, color);
        }
    }
    const bool marginA = isPointOnMargin(px[numPoints-1], py[numPoints-1]);
    const bool marginB = isPointOnMargin(px[0], py[0]);
    const uint16_t color = (marginA && marginB) ? fillColor : borderColor;
    const int x0 = px[numPoints-1] + xOffset;
    const int y0 = py[numPoints-1] + yOffset;
    const int x1 = px[0] + xOffset;
    const int y1 = py[0] + yOffset;
    if (x0 >= 0 && x0 <= TILE_SIZE + xOffset && y0 >= 0 && y0 <= TILE_SIZE + yOffset &&
        x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset) {
        if (!(marginA && marginB && !fillPolygons)) map.drawLine(x0, y0, x1, y1, color);
    }
}   

// --- Cache & Pool Management ---

void Maps::initTileCache() {
    tileCache.clear(); tileCache.reserve(maxCachedTiles); cacheAccessCounter = 0;
    ESP_LOGI(TAG, "Tile cache initialized with %zu tiles capacity", maxCachedTiles);
}
uint32_t Maps::calculateTileHash(const char* filePath) {
    uint32_t hash = 0; const char* p = filePath; while (*p) { hash = hash * 31 + *p; p++; } return hash;
}
bool Maps::getCachedTile(const char* filePath, TFT_eSprite& target, int16_t xOffset, int16_t yOffset) {
    if (maxCachedTiles == 0) return false; 
    uint32_t tileHash = calculateTileHash(filePath);
    for (auto& cachedTile : tileCache) {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) {
            cachedTile.lastAccess = ++cacheAccessCounter;
            target.pushImage(xOffset, yOffset, tileWidth, tileHeight, cachedTile.sprite->frameBuffer(0));
            return true;
        }
    }
    return false;
}
void Maps::addToCache(const char* filePath, TFT_eSprite& source) {
    if (maxCachedTiles == 0) return; 
    uint32_t tileHash = calculateTileHash(filePath);
    for (auto& cachedTile : tileCache) {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) { cachedTile.lastAccess = ++cacheAccessCounter; return; }
    }
    if (tileCache.size() >= maxCachedTiles) evictLRUTile();
    CachedTile newEntry; newEntry.sprite = new TFT_eSprite(&tft); newEntry.sprite->createSprite(tileWidth, tileHeight);
    newEntry.sprite->pushImage(0, 0, tileWidth, tileHeight, source.frameBuffer(0));
    newEntry.tileHash = tileHash; newEntry.lastAccess = ++cacheAccessCounter; newEntry.isValid = true;
    strncpy(newEntry.filePath, filePath, sizeof(newEntry.filePath) - 1); newEntry.filePath[sizeof(newEntry.filePath) - 1] = '\0';
    tileCache.push_back(newEntry);
}
void Maps::evictLRUTile() {
    if (tileCache.empty()) return;
    auto lruIt = tileCache.begin(); uint32_t bestScore = lruIt->lastAccess; size_t bestMemoryUsage = 0;
    for (auto it = tileCache.begin(); it != tileCache.end(); ++it) {
        size_t tileMemory = (it->sprite) ? (tileWidth * tileHeight * 2) : 0;
        uint32_t score = it->lastAccess + (tileMemory / 1024); 
        if (score < bestScore || (score == bestScore && tileMemory > bestMemoryUsage)) {
            bestScore = score; bestMemoryUsage = tileMemory; lruIt = it;
        }
    }
    if (lruIt->sprite) { lruIt->sprite->deleteSprite(); delete lruIt->sprite; }
    tileCache.erase(lruIt);
}
void Maps::clearTileCache() {
    for (auto& cachedTile : tileCache) { if (cachedTile.sprite) { cachedTile.sprite->deleteSprite(); delete cachedTile.sprite; } }
    tileCache.clear(); cacheAccessCounter = 0;
}
size_t Maps::getCacheMemoryUsage() {
    size_t memoryUsage = 0;
    for (const auto& cachedTile : tileCache) { if (cachedTile.isValid && cachedTile.sprite) memoryUsage += tileWidth * tileHeight * 2; }
    return memoryUsage;
}

void Maps::initUnifiedPool() {
    ESP_LOGI(TAG, "Initializing unified memory pool...");
    if (unifiedPoolMutex == nullptr) { unifiedPoolMutex = xSemaphoreCreateMutex(); if (unifiedPoolMutex == nullptr) return; }
    #ifdef BOARD_HAS_PSRAM
        size_t psramFree = ESP.getFreePsram(); maxUnifiedPoolEntries = std::min(static_cast<size_t>(100), psramFree / (1024 * 32));
    #else
        size_t ramFree = ESP.getFreeHeap(); maxUnifiedPoolEntries = std::min(static_cast<size_t>(25), ramFree / (1024 * 64));
    #endif
    unifiedPool.clear(); unifiedPool.reserve(maxUnifiedPoolEntries); unifiedPoolHitCount = 0; unifiedPoolMissCount = 0;
    ESP_LOGI(TAG, "Unified memory pool initialized");
}

void* Maps::unifiedAlloc(size_t size, uint8_t type) {
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : unifiedPool) {
            if (!entry.isInUse && entry.size >= size) {
                entry.isInUse = true; entry.allocationCount++; entry.type = type; unifiedPoolHitCount++; xSemaphoreGive(unifiedPoolMutex); return entry.ptr;
            }
        }
        if (unifiedPool.size() < maxUnifiedPoolEntries) {
            void* ptr = nullptr;
            #ifdef BOARD_HAS_PSRAM
                ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            #else
                ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);
            #endif
            if (ptr) {
                UnifiedPoolEntry entry; entry.ptr = ptr; entry.size = size; entry.isInUse = true; entry.allocationCount = 1; entry.type = type;
                unifiedPool.push_back(entry); unifiedPoolHitCount++; xSemaphoreGive(unifiedPoolMutex); return ptr;
            }
        }
        unifiedPoolMissCount++; xSemaphoreGive(unifiedPoolMutex);
    }
    unifiedPoolMissCount++;
    #ifdef BOARD_HAS_PSRAM
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    #else
        return heap_caps_malloc(size, MALLOC_CAP_8BIT);
    #endif
}

void Maps::unifiedDealloc(void* ptr) {
    if (!ptr) return;
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : unifiedPool) {
            if (entry.ptr == ptr && entry.isInUse) { entry.isInUse = false; xSemaphoreGive(unifiedPoolMutex); return; }
        }
        xSemaphoreGive(unifiedPoolMutex);
    }
    heap_caps_free(ptr);
}

void Maps::initBatchRendering() {
    #ifdef BOARD_HAS_PSRAM
        size_t psramFree = ESP.getFreePsram();
        if (psramFree >= 4 * 1024 * 1024) maxBatchSize = 512; 
        else if (psramFree >= 2 * 1024 * 1024) maxBatchSize = 256;
        else maxBatchSize = 128;
    #else
        maxBatchSize = 64;
    #endif
    activeBatch = nullptr; batchRenderCount = 0; batchOptimizationCount = 0; batchFlushCount = 0;
    ESP_LOGI(TAG, "Batch rendering initialized");
}

void Maps::createRenderBatch(size_t capacity) {
    if (activeBatch) { if (activeBatch->segments) delete[] activeBatch->segments; delete activeBatch; activeBatch = nullptr; }
    activeBatch = new RenderBatch(); activeBatch->segments = new LineSegment[capacity];
    activeBatch->count = 0; activeBatch->capacity = capacity; activeBatch->color = 0;
}

void Maps::addToBatch(int x0, int y0, int x1, int y1, uint16_t color) {
    if (!activeBatch) createRenderBatch(maxBatchSize);
    if (!canBatch(color) || activeBatch->count >= activeBatch->capacity) return; 
    activeBatch->segments[activeBatch->count] = {x0, y0, x1, y1, color};
    activeBatch->count++;
    if (activeBatch->count == 1) activeBatch->color = color;
}

void Maps::flushBatch(TFT_eSprite& map, int& optimizations) {
    if (!activeBatch || activeBatch->count == 0) return;
    batchFlushCount++;
    size_t optimizationThreshold = maxBatchSize / 16;
    if (activeBatch->count > optimizationThreshold) { batchOptimizationCount++; optimizations++; }
    for (size_t i = 0; i < activeBatch->count; i++) {
        const LineSegment& segment = activeBatch->segments[i];
        map.drawLine(segment.x0, segment.y0, segment.x1, segment.y1, segment.color);
    }
    batchRenderCount++; activeBatch->count = 0; activeBatch->color = 0;
}

bool Maps::canBatch(uint16_t color) {
    if (!activeBatch) return true;
    return (activeBatch->count == 0) || (activeBatch->color == color);
}

size_t Maps::getOptimalBatchSize() { return maxBatchSize; }

// --- renderTile (Modified for width) ---

bool Maps::renderTile(const char* path, const int16_t xOffset, const int16_t yOffset, TFT_eSprite &map)
{
    static bool isPaletteLoaded = false;
    static bool unifiedPoolLogged = false;
    static bool unifiedPolylineLogged = false;
    static bool unifiedPolygonLogged = false;
    static bool unifiedPolygonsLogged = false;

    if (!isPaletteLoaded) isPaletteLoaded = Maps::loadPalette("/sdcard/VECTMAP/palette.bin");
    if (!path || path[0] == '\0') return false;
    if (getCachedTile(path, map, xOffset, yOffset)) return true; 

    FILE* file = fopen(path, "rb");
    if (!file) return false;
    
    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    MemoryGuard<uint8_t> dataGuard(fileSize, 0);
    uint8_t* data = dataGuard.get();
    if (!data) { fclose(file); return false; }
    if (!unifiedPoolLogged) unifiedPoolLogged = true;

    const size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) return false;

    currentMemoryUsage = ESP.getFreeHeap();
    if (currentMemoryUsage > peakMemoryUsage) peakMemoryUsage = currentMemoryUsage;

    size_t offset = 0;
    const size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = RGB332ToRGB565(current_color);

    const uint32_t num_cmds = readVarint(data, offset, dataSize);
    if (num_cmds == 0) return false;

    const size_t optimalBatchSize = getOptimalBatchSize();
    createRenderBatch(optimalBatchSize);
    map.initDMA();
    
    int batchCount = 0;
    totalMemoryAllocations++;
    int executed = 0;
    int totalLines = 0;
    int batchFlushes = 0;
    int batchOptimizations = 0;
    unsigned long renderStart = millis();
    int polygonCommands = 0;

    auto flushCurrentBatch = [&]() {
        if (batchCount == 0) return;
        totalLines += batchCount;
        batchFlushes++;
        flushBatch(map, batchOptimizations);
        batchCount = 0;
    };

    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) 
    {
        if (offset >= dataSize) break;
        const size_t cmdStartOffset = offset;
        const uint32_t cmdType = readVarint(data, offset, dataSize);

        if (cmd_idx > 0 && cmd_idx % 50 == 0) flushCurrentBatch();

        bool isLineCommand = false;

        switch (cmdType) 
        {
            case SET_LAYER:
                readVarint(data, offset, dataSize);
                break;
            case SET_COLOR:
                flushCurrentBatch();
                if (offset < dataSize) {
                    current_color = data[offset++];
                    currentDrawColor = RGB332ToRGB565(current_color);
                    executed++;
                }
                break;
            case SET_COLOR_INDEX:
                flushCurrentBatch();
                {
                    const uint32_t color_index = readVarint(data, offset, dataSize);
                    current_color = paletteToRGB332(color_index);
                    currentDrawColor = RGB332ToRGB565(current_color);
                    executed++;
                }
                break;
            case DRAW_LINE:
                {
                    // Leer ancho varint
                    const uint32_t widthVar = readVarint(data, offset, dataSize);
                    float width = (float)std::max(1U, widthVar);
                    
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t dy = readZigzag(data, offset, dataSize);
                    const int32_t x2 = x1 + dx;
                    const int32_t y2 = y1 + dy;
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    
                    if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) {
                        if (width <= 1.0f) {
                             addToBatch(px1, py1, px2, py2, currentDrawColor);
                             batchCount++;
                        } else {
                             flushCurrentBatch();
                             map.drawWideLine(px1, py1, px2, py2, width, currentDrawColor);
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case DRAW_POLYLINE:
                {
                    // Leer ancho varint
                    const uint32_t widthVar = readVarint(data, offset, dataSize);
                    float width = (float)std::max(1U, widthVar);

                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 2) {
                        MemoryGuard<int> pxGuard(numPoints, 6);
                        MemoryGuard<int> pyGuard(numPoints, 6);
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py) continue;
                        if (!unifiedPolylineLogged) unifiedPolylineLogged = true;
                        unifiedPoolHitCount += 2;
                        
                        int32_t prevX = readZigzag(data, offset, dataSize);
                        int32_t prevY = readZigzag(data, offset, dataSize);
                        px[0] = uint16ToPixel(prevX) + xOffset;
                        py[0] = uint16ToPixel(prevY) + yOffset;
                        
                        for (uint32_t i = 1; i < numPoints; ++i) {
                            const int32_t deltaX = readZigzag(data, offset, dataSize);
                            const int32_t deltaY = readZigzag(data, offset, dataSize);
                            prevX += deltaX;
                            prevY += deltaY;
                            px[i] = uint16ToPixel(prevX) + xOffset;
                            py[i] = uint16ToPixel(prevY) + yOffset;
                        }
                        
                        for (uint32_t i = 1; i < numPoints; ++i) {
                             if (shouldDrawLine(px[i-1] - xOffset, py[i-1] - yOffset, px[i] - xOffset, py[i] - yOffset)) {
                                 if (width <= 1.0f) {
                                     addToBatch(px[i-1], py[i-1], px[i], py[i], currentDrawColor);
                                     batchCount++;
                                 } else {
                                     flushCurrentBatch();
                                     map.drawWideLine(px[i-1], py[i-1], px[i], py[i], width, currentDrawColor);
                                 }
                             }
                        }
                        executed++;
                        isLineCommand = true;
                    } else {
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                            readZigzag(data, offset, dataSize); readZigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case DRAW_STROKE_POLYGON:
            case OPTIMIZED_POLYGON:
            case HOLLOW_POLYGON:
                flushCurrentBatch();
                {
                    // Leer ancho varint
                    const uint32_t widthVar = readVarint(data, offset, dataSize);
                    float width = (float)std::max(1U, widthVar);
                    
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 3) {
                        MemoryGuard<int> pxGuard(numPoints, 6);
                        MemoryGuard<int> pyGuard(numPoints, 6);
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py) {
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                                readZigzag(data, offset, dataSize); readZigzag(data, offset, dataSize);
                            }
                            break;
                        }
                        if (!unifiedPolygonLogged) unifiedPolygonLogged = true;

                        const int32_t firstX = readZigzag(data, offset, dataSize);
                        const int32_t firstY = readZigzag(data, offset, dataSize);
                        px[0] = uint16ToPixel(firstX);
                        py[0] = uint16ToPixel(firstY);
                        int prevX = firstX;
                        int prevY = firstY;
                        for (uint32_t i = 1; i < numPoints; ++i) {
                            const int32_t deltaX = readZigzag(data, offset, dataSize);
                            const int32_t deltaY = readZigzag(data, offset, dataSize);
                            prevX += deltaX; prevY += deltaY;
                            px[i] = uint16ToPixel(prevX);
                            py[i] = uint16ToPixel(prevY);
                        }
                        
                        // Polygon filling is independent of stroke width
                        if (fillPolygons && numPoints >= 3 && cmdType != HOLLOW_POLYGON) 
                            fillPolygonGeneral(map, px, py, numPoints, currentDrawColor, xOffset, yOffset);

                        const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                        
                        // Handle stroke width
                        if (width <= 1.0f) {
                            drawPolygonBorder(map, px, py, numPoints, borderColor, currentDrawColor, xOffset, yOffset);
                        } else {
                             for (uint32_t i = 0; i < numPoints; ++i) {
                                uint32_t next = (i + 1) % numPoints;
                                int x0 = px[i] + xOffset; int y0 = py[i] + yOffset;
                                int x1 = px[next] + xOffset; int y1 = py[next] + yOffset;
                                map.drawWideLine(x0, y0, x1, y1, width, borderColor);
                            }
                        }
                        executed++;
                    } else {
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                            readZigzag(data, offset, dataSize); readZigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            
            // ... [Resto de comandos originales sin cambios] ...
            case DRAW_STROKE_POLYGONS:
                flushCurrentBatch();
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    int32_t accumX = 0, accumY = 0;
                    if (numPoints >= 3) {
                        MemoryGuard<int> pxGuard(numPoints, 6);
                        MemoryGuard<int> pyGuard(numPoints, 6);
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py) {
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                                readZigzag(data, offset, dataSize); readZigzag(data, offset, dataSize);
                            }
                            break;
                        }
                        if (!unifiedPolygonsLogged) unifiedPolygonsLogged = true;
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                            if (i == 0) {
                                accumX = readZigzag(data, offset, dataSize);
                                accumY = readZigzag(data, offset, dataSize);
                            } else {
                                const int32_t deltaX = readZigzag(data, offset, dataSize);
                                const int32_t deltaY = readZigzag(data, offset, dataSize);
                                accumX += deltaX; accumY += deltaY;
                            }
                            px[i] = uint16ToPixel(accumX);
                            py[i] = uint16ToPixel(accumY);
                        }
                        if (fillPolygons && numPoints >= 3) {
                            fillPolygonGeneral(map, px, py, numPoints, currentDrawColor, xOffset, yOffset);
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            drawPolygonBorder(map, px, py, numPoints, borderColor, currentDrawColor, xOffset, yOffset);
                            polygonCommands++;
                            executed++;
                        }
                    } else {
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) {
                            readZigzag(data, offset, dataSize); readZigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case DRAW_HORIZONTAL_LINE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t x2 = x1 + dx;
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py = uint16ToPixel(y) + yOffset;
                    if (shouldDrawLine(px1 - xOffset, py - yOffset, px2 - xOffset, py - yOffset)) {
                        addToBatch(px1, py, px2, py, currentDrawColor);
                        batchCount++;
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case DRAW_VERTICAL_LINE:
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t dy = readZigzag(data, offset, dataSize);
                    const int32_t y2 = y1 + dy;
                    const int px = uint16ToPixel(x) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    if (shouldDrawLine(px - xOffset, py1 - yOffset, px - xOffset, py2 - yOffset)) {
                        addToBatch(px, py1, px, py2, currentDrawColor);
                        batchCount++;
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case RECTANGLE:
                flushCurrentBatch();
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t dy = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int pwidth = uint16ToPixel(dx);
                    const int pheight = uint16ToPixel(dy);
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        pwidth > 0 && pheight > 0 &&
                        !isPointOnMargin(px1, py1) &&
                        !isPointOnMargin(px1 + pwidth, py1 + pheight)) {
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, borderColor);
                        } else map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case SIMPLE_RECTANGLE:
                flushCurrentBatch();
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t width = readZigzag(data, offset, dataSize);
                    const int32_t height = readZigzag(data, offset, dataSize);
                    const int px = uint16ToPixel(x);
                    const int py = uint16ToPixel(y);
                    const int pwidth = uint16ToPixel(width);
                    const int pheight = uint16ToPixel(height);
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE &&
                        pwidth > 0 && pheight > 0 &&
                        px + pwidth <= TILE_SIZE && py + pheight <= TILE_SIZE) {
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, borderColor);
                        } else map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case OPTIMIZED_RECTANGLE:
                flushCurrentBatch();
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t width = readZigzag(data, offset, dataSize);
                    const int32_t height = readZigzag(data, offset, dataSize);
                    const int px = uint16ToPixel(x);
                    const int py = uint16ToPixel(y);
                    const int pwidth = uint16ToPixel(width);
                    const int pheight = uint16ToPixel(height);
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE &&
                        pwidth > 0 && pheight > 0 &&
                        px + pwidth <= TILE_SIZE && py + pheight <= TILE_SIZE) {
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, borderColor);
                        } else map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case STRAIGHT_LINE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t dy = readZigzag(data, offset, dataSize);
                    const int32_t x2 = x1 + dx;
                    const int32_t y2 = y1 + dy;
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) {
                        addToBatch(px1, py1, px2, py2, currentDrawColor);
                        batchCount++;
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case CIRCLE:
                flushCurrentBatch();
                {
                    const int32_t center_x = readZigzag(data, offset, dataSize);
                    const int32_t center_y = readZigzag(data, offset, dataSize);
                    const int32_t radius = readZigzag(data, offset, dataSize);
                    const int pcx = uint16ToPixel(center_x);
                    const int pcy = uint16ToPixel(center_y);
                    const int pradius = uint16ToPixel(radius);
                    if (pcx >= 0 && pcx <= TILE_SIZE && pcy >= 0 && pcy <= TILE_SIZE && pradius > 0 &&
                        !isPointOnMargin(pcx, pcy)) {
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, borderColor);
                            executed++;
                        } else map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case SIMPLE_CIRCLE:
                flushCurrentBatch();
                {
                    const int32_t center_x = readZigzag(data, offset, dataSize);
                    const int32_t center_y = readZigzag(data, offset, dataSize);
                    const int32_t radius = readZigzag(data, offset, dataSize);
                    const int pcx = uint16ToPixel(center_x);
                    const int pcy = uint16ToPixel(center_y);
                    const int pradius = uint16ToPixel(radius);
                    if (pcx >= 0 && pcx <= TILE_SIZE && pcy >= 0 && pcy <= TILE_SIZE && pradius > 0 &&
                        pcx + pradius <= TILE_SIZE && pcy + pradius <= TILE_SIZE &&
                        pcx - pradius >= 0 && pcy - pradius >= 0) {
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, borderColor);
                        } else map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case SIMPLE_TRIANGLE:
                flushCurrentBatch();
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t x3 = readZigzag(data, offset, dataSize);
                    const int32_t y3 = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    const int px3 = uint16ToPixel(x3) + xOffset;
                    const int py3 = uint16ToPixel(y3) + yOffset;
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE &&
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) {
                        if (fillPolygons) {
                            map.fillTriangle(px1, py1, px2, py2, px3, py3, currentDrawColor);
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.drawLine(px1, py1, px2, py2, borderColor);
                            map.drawLine(px2, py2, px3, py3, borderColor);
                            map.drawLine(px3, py3, px1, py1, borderColor);
                        } else {
                            map.drawLine(px1, py1, px2, py2, currentDrawColor);
                            map.drawLine(px2, py2, px3, py3, currentDrawColor);
                            map.drawLine(px3, py3, px1, py1, currentDrawColor);
                        }
                        executed++;
                    }
                }
                break;
            case OPTIMIZED_TRIANGLE:
                flushCurrentBatch();
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t x3 = readZigzag(data, offset, dataSize);
                    const int32_t y3 = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    const int px3 = uint16ToPixel(x3) + xOffset;
                    const int py3 = uint16ToPixel(y3) + yOffset;
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE &&
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) {
                        if (fillPolygons) {
                            map.fillTriangle(px1, py1, px2, py2, px3, py3, currentDrawColor);
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.drawLine(px1, py1, px2, py2, borderColor);
                            map.drawLine(px2, py2, px3, py3, borderColor);
                            map.drawLine(px3, py3, px1, py1, borderColor);
                        } else {
                            map.drawLine(px1, py1, px2, py2, currentDrawColor);
                            map.drawLine(px2, py2, px3, py3, currentDrawColor);
                            map.drawLine(px3, py3, px1, py1, currentDrawColor);
                        }
                        executed++;
                    }
                }
                break;
            case DASHED_LINE:
                flushCurrentBatch();
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t dashLength = readVarint(data, offset, dataSize);
                    const int32_t gapLength = readVarint(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) {
                        int dx = abs(px2 - px1); int dy = abs(py2 - py1);
                        int sx = (px1 < px2) ? 1 : -1; int sy = (py1 < py2) ? 1 : -1;
                        int err = dx - dy;
                        int currentX = px1, currentY = py1, dashCounter = 0;
                        while (true) {
                            if (dashCounter % (dashLength + gapLength) < dashLength)
                                map.drawPixel(currentX, currentY, currentDrawColor);
                            dashCounter++;
                            if (currentX == px2 && currentY == py2) break;
                            int e2 = 2 * err;
                            if (e2 > -dy) { err -= dy; currentX += sx; }
                            if (e2 < dx) { err += dx; currentY += sy; }
                        }
                        executed++;
                    }
                }
                break;
            case DOTTED_LINE:
                flushCurrentBatch();
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t dotSpacing = readVarint(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int py1 = uint16ToPixel(y1) + yOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py2 = uint16ToPixel(y2) + yOffset;
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) {
                        int dx = abs(px2 - px1); int dy = abs(py2 - py1);
                        int sx = (px1 < px2) ? 1 : -1; int sy = (py1 < py2) ? 1 : -1;
                        int err = dx - dy;
                        int currentX = px1, currentY = py1, dotCounter = 0;
                        while (true) {
                            if (dotCounter % dotSpacing == 0)
                                map.drawPixel(currentX, currentY, currentDrawColor);
                            dotCounter++;
                            if (currentX == px2 && currentY == py2) break;
                            int e2 = 2 * err;
                            if (e2 > -dy) { err -= dy; currentX += sx; }
                            if (e2 < dx) { err += dx; currentY += sy; }
                        }
                        executed++;
                    }
                }
                break;
            case GRID_PATTERN:
                flushCurrentBatch();
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t width = readVarint(data, offset, dataSize);
                    const int32_t spacing = readVarint(data, offset, dataSize);
                    const int32_t count = readVarint(data, offset, dataSize);
                    const int32_t direction = readVarint(data, offset, dataSize);
                    const int px = uint16ToPixel(x) + xOffset;
                    const int py = uint16ToPixel(y) + yOffset;
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE) {
                        if (direction == 0) {
                            for (int i = 0; i < count; i++) {
                                int lineY = py + (i * spacing);
                                map.drawLine(px, lineY, px + width, lineY, currentDrawColor);
                            }
                        } else {
                            for (int i = 0; i < count; i++) {
                                int lineX = px + (i * spacing);
                                map.drawLine(lineX, py, lineX, py + width, currentDrawColor);
                            }
                        }
                        executed++;
                    }
                }
                break;            
            default:
                flushCurrentBatch();
                if (offset < dataSize - 4) offset += 4;
                break;
        }
        if (!isLineCommand) flushCurrentBatch();
        if (offset <= cmdStartOffset) break;
    }
    flushCurrentBatch();
    
    if (activeBatch) {
        if (activeBatch->segments) delete[] activeBatch->segments;
        delete activeBatch; activeBatch = nullptr;
    }

    if (executed == 0) return false;
    addToCache(path, map);
    unsigned long renderTime = millis() - renderStart;
    return true;
}