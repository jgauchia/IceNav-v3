/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.4
 * @date 2025-12
 */

#include "maps.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include "esp_timer.h"
#include "esp_system.h"

static inline uint32_t millis_idf() { return (uint32_t)(esp_timer_get_time() / 1000); }

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData; /**< Vector containing track waypoints */
const char* TAG = "Maps";

uint16_t Maps::currentDrawColor = TFT_WHITE;
uint8_t Maps::PALETTE[256] = {0};
uint32_t Maps::PALETTE_SIZE = 0;

// Tile cache system static variables
std::vector<Maps::CachedTile> Maps::tileCache;
size_t Maps::maxCachedTiles = 0;
uint32_t Maps::cacheAccessCounter = 0;


// Unified memory pool system static variables (experimental)
std::vector<Maps::UnifiedPoolEntry> Maps::unifiedPool;
SemaphoreHandle_t Maps::unifiedPoolMutex = nullptr;
size_t Maps::maxUnifiedPoolEntries = 0;
uint32_t Maps::unifiedPoolHitCount = 0;
uint32_t Maps::unifiedPoolMissCount = 0;

// Memory monitoring static variables
uint32_t Maps::totalMemoryAllocations = 0;
uint32_t Maps::totalMemoryDeallocations = 0;
uint32_t Maps::peakMemoryUsage = 0;
uint32_t Maps::currentMemoryUsage = 0;
uint32_t Maps::poolEfficiencyScore = 0;
uint32_t Maps::lastStatsUpdate = 0;

// Polygon optimization system static variables
bool Maps::polygonCullingEnabled = true;
bool Maps::optimizedScanlineEnabled = true;
uint32_t Maps::polygonRenderCount = 0;
uint32_t Maps::polygonCulledCount = 0;
uint32_t Maps::polygonOptimizedCount = 0;


/**
 * @brief Map Class constructor
 *
 * @details Initializes the Maps class with default polygon filling enabled and logs completion status.
 */
Maps::Maps() : fillPolygons(true) 
{
    ESP_LOGI(TAG, "Maps constructor completed");
}

/**
 * @brief Get pixel X position from OpenStreetMap Render map longitude
 *
 * @details Converts a longitude value to the corresponding pixel X position
 * 			for a given zoom level and tile size in an OpenStreetMap render map.
 *
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
 * @brief Get pixel Y position from OpenStreetMap Render map latitude
 *
 * @details Converts a latitude value to the corresponding pixel Y position
 * 			for a given zoom level and tile size in an OpenStreetMap render map.
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
    float scale = (1 << zoom) * tileSize;
    return static_cast<uint16_t>(((1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * scale)) % tileSize;
}

/**
 * @brief Get TileX for OpenStreetMap files
 *
 * @details Converts a longitude value to the corresponding tile X index (folder) for OpenStreetMap files,
 * 			for a given zoom level.
 *
 * @param f_lon Longitude coordinate.
 * @param zoom Zoom level.
 * @return X tile index (folder).
 */
uint32_t Maps::lon2tilex(float f_lon, uint8_t zoom)
{
    float rawTile = (f_lon + 180.0f) / 360.0f * (1 << zoom);
    rawTile += 1e-6f;
    return static_cast<uint32_t>(rawTile);
}

/**
 * @brief Get TileY for OpenStreetMap files
 *
 * @details Converts a latitude value to the corresponding tile Y index (file) for OpenStreetMap files,
 * 			for a given zoom level.
 *
 * @param f_lat Latitude coordinate.
 * @param zoom Zoom level.
 * @return Y tile index (file).
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
 * @brief Get Longitude from OpenStreetMap files
 *
 * @details Converts a tile X index to the corresponding longitude value for OpenStreetMap files,
 * 			for a given zoom level.
 *
 * @param tileX Tile X index.
 * @param zoom Zoom level.
 * @return Longitude coordinate.
 */
float Maps::tilex2lon(uint32_t tileX, uint8_t zoom)
{
    return static_cast<float>(tileX) * 360.0f / (1 << zoom) - 180.0f;
}

/**
 * @brief Get Latitude from OpenStreetMap files
 *
 * @details Converts a tile Y index to the corresponding latitude value for OpenStreetMap files,
 * 			for a given zoom level.
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
 * @brief Get the map tile structure from GPS Coordinates
 *
 * @details Constructs a MapTile structure from the given GPS coordinates, zoom level, and optional tile offsets.
 *
 * @param lon Longitude coordinate.
 * @param lat Latitude coordinate.
 * @param zoomLevel Zoom level.
 * @param offsetX Tile offset X.
 * @param offsetY Tile offset Y.
 * @return MapTile structure containing tile indices, file path, zoom, and GPS coordinates.
 */
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

/**
 * @brief Get min and max longitude and latitude from tile
 *
 * @details Returns the geographic boundaries (min/max longitude and latitude) for the specified tile coordinates and zoom level.
 *
 * @param tileX Tile X coordinate.
 * @param tileY Tile Y coordinate.
 * @param zoom Zoom level.
 * @return tileBounds Structure containing min and max longitude and latitude.
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
 * @details Checks if the latitude and longitude are within the specified tileBounds.
 *
 * @param lat Latitude to check.
 * @param lon Longitude to check.
 * @param bound Map bounds to check against.
 * @return true if the coordinate is inside the bounds, false otherwise.
 */
bool Maps::isCoordInBounds(float lat, float lon, tileBounds bound)
{
    return (lat >= bound.lat_min && lat <= bound.lat_max &&
            lon >= bound.lon_min && lon <= bound.lon_max);
}

/**
 * @brief Convert GPS Coordinates to screen position (with offsets)
 *
 * @details Converts the given longitude and latitude to screen coordinates at a specific zoom level and tile size.
 *
 * @param lon Longitude in degrees.
 * @param lat Latitude in degrees.
 * @param zoomLevel Zoom level of the map.
 * @param tileSize Size of a single map tile in pixels.
 * @return ScreenCoord Screen position (x, y) corresponding to the GPS coordinates.
 */
Maps::ScreenCoord Maps::coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize)
{
    ScreenCoord data;
    data.posX = Maps::lon2posx(lon, zoomLevel, tileSize);
    data.posY = Maps::lat2posy(lat, zoomLevel, tileSize);
    return data;
}

/**
 * @brief Get position X,Y in render map for a coordinate
 *
 * @details Converts latitude and longitude into pixel X,Y positions on the rendered map, given the map bounds.
 *
 * @param lat Latitude of the coordinate.
 * @param lon Longitude of the coordinate.
 * @param bound Map bounds (min/max lat/lon) of the tile.
 * @param pixelX Pointer to store the computed X position on the map.
 * @param pixelY Pointer to store the computed Y position on the map.
 */
void Maps::coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY)
{
    float lon_ratio = (lon - bound.lon_min) / (bound.lon_max - bound.lon_min);
    float lat_ratio = (bound.lat_max - lat) / (bound.lat_max - bound.lat_min);

    *pixelX = (uint16_t)(lon_ratio * Maps::tileWidth);
    *pixelY = (uint16_t)(lat_ratio * Maps::tileHeight);
}

/**
 * @brief Load No Map Image
 *
 * @details Draws a "No Map Found" PNG image 
 *
 * @param map Reference to the TFT_eSprite map object.
 */
void Maps::showNoMap(TFT_eSprite &map)
{
    map.drawPngFile(noMapFile, (Maps::mapScrWidth / 2) - 50, (Maps::mapScrHeight / 2) - 50);
    map.drawCenterString("NO MAP FOUND", (Maps::mapScrWidth / 2), (Maps::mapScrHeight >> 1) + 65, &fonts::DejaVu18);
}

/**
 * @brief Init map size
 *
 * @details Initializes the map screen size and allocates buffer space for rendering the map.
 * 			Also resets map tile state and navigation arrow position, and sets default map bounds.
 *
 * @param mapHeight  Screen map size height.
 * @param mapWidth   Screen map size width.
 */
void Maps::initMap(uint16_t mapHeight, uint16_t mapWidth)
{
    Maps::mapScrHeight = mapHeight;
    Maps::mapScrWidth = mapWidth;

    // Reserve PSRAM for buffer map
    Maps::mapTempSprite.deleteSprite();
    Maps::mapTempSprite.createSprite(tileHeight, tileWidth);

    Maps::oldMapTile = {};     // Old Map tile coordinates and zoom
    Maps::currentMapTile = {}; // Current Map tile coordinates and zoom
    Maps::roundMapTile = {};    // Boundaries Map tiles
    Maps::navArrowPosition = {0, 0};              // Map Arrow position

    Maps::totalBounds = {90.0, -90.0, 180.0, -180.0};
    
    // Initialize tile cache system
    initTileCache();
    
    // Initialize polygon optimizations
    polygonCullingEnabled = true;
    optimizedScanlineEnabled = false;
    polygonRenderCount = 0;
    polygonCulledCount = 0;
    polygonOptimizedCount = 0;
}

/**
 * @brief Delete map screen and release PSRAM
 *
 * @details Deletes the main map sprite to free up PSRAM.
 */
void Maps::deleteMapScrSprites()
{
    Maps::mapSprite.deleteSprite();
     
    // Clear tile cache to free memory
    clearTileCache();
}

/**
 * @brief Create map screen 
 *
 * @details Creates the main map sprite with the current screen width and height, allocating memory for rendering.
 */
void Maps::createMapScrSprites()
{
    Maps::mapBuffer = Maps::mapSprite.createSprite(Maps::mapScrWidth, Maps::mapScrHeight);
}

/**
 * @brief Generate map
 *
 * @details Generates the main map by compositing the center and surrounding tiles based on the current zoom level.
 * 			Handles missing tiles, updates map bounds, overlays missing map notifications, and draws tracks if available.
 *
 * @param zoom Zoom Level
 */
void Maps::generateMap(uint8_t zoom)
{
    // Clear cache if zoom level changed (tiles are not compatible between zoom levels)
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

    // Detects if tile changes from actual GPS position
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
            Maps::totalBounds = Maps::getTileBounds(
                Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);

            const int8_t startX = -1;
            const int8_t startY = -1;

            for (int8_t y = startY; y <= startY + 2; y++)
            {
                const int16_t offsetY = (y - startY) * size;

                for (int8_t x = startX; x <= startX + 2; x++)
                {
                    if (x == 0 && y == 0) continue; // Skip center tile

                    const int16_t offsetX = (x - startX) * size;

                    Maps::roundMapTile = getMapTile(
                        Maps::currentMapTile.lon, Maps::currentMapTile.lat,
                        Maps::zoomLevel, x, y);
 
                    if (mapSet.vectorMap)
                        foundRoundMap = renderTile(Maps::roundMapTile.file, offsetX, offsetY,Maps::mapTempSprite);
                    else
                        foundRoundMap = Maps::mapTempSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);

                    if (!foundRoundMap)
                    {
                        Maps::mapTempSprite.fillRect(offsetX, offsetY, size, size, TFT_BLACK);
                        Maps::mapTempSprite.drawPngFile(noMapFile,
                            offsetX + size / 2 - 50,
                            offsetY + size / 2 - 50);
                        missingMap = true;
                    }
                    else
                    {
                        const tileBounds currentBounds = Maps::getTileBounds(
                            Maps::roundMapTile.tilex, Maps::roundMapTile.tiley, Maps::zoomLevel);

                        if (currentBounds.lat_min < Maps::totalBounds.lat_min)
                            Maps::totalBounds.lat_min = currentBounds.lat_min;
                        if (currentBounds.lat_max > Maps::totalBounds.lat_max)
                            Maps::totalBounds.lat_max = currentBounds.lat_max;
                        if (currentBounds.lon_min < Maps::totalBounds.lon_min)
                            Maps::totalBounds.lon_min = currentBounds.lon_min;
                        if (currentBounds.lon_max > Maps::totalBounds.lon_max)
                            Maps::totalBounds.lon_max = currentBounds.lon_max;
                    }
                }
            }

            if (!missingMap)
            {
                if (Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
                {
                    Maps::coords2map(Maps::destLat, Maps::destLon,
                        Maps::totalBounds, &wptPosX, &wptPosY);
                }
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
                    Maps::mapTempSprite.drawWideLine(x1, y1, x2, y2, 2, TFT_BLUE);
                }
            }
            
            // Prefetch adjacent tiles for faster scrolling
            prefetchAdjacentTiles(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);
        }
    }
}

/**
 * @brief Display Map
 *
 * @details Displays the map on the screen with proper rotation, waypoint overlay, and GPS navigation arrow positioning.
 *          Handles both GPS-following mode and manual panning mode with appropriate pivot points and rotations.
 */
void Maps::displayMap()
{
    if (!Maps::isMapFound)
    {
        if (Maps::scrollUpdated && !Maps::followGps)
            Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
        else
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

    const uint16_t size = Maps::mapTileSize;

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

/**
 * @brief Set Waypoint coords in Map
 *
 * @details Sets the latitude and longitude for the waypoint on the map.
 *
 * @param wptLat Waypoint Latitude
 * @param wptLon Waypoint Longitude
 */
void Maps::setWaypoint(float wptLat, float wptLon)
{
    Maps::destLat = wptLat;
    Maps::destLon = wptLon;
}

/**
 * @brief Refresh current map
 *
 * @details Resets the old map tile and marks the position as moved, causing the map to be updated/redrawn.
 */
void Maps::updateMap()
{
    Maps::oldMapTile = {};
}

/**
 * @brief Pan current map
 *
 * @details Moves the map view by the given tile offsets in the X (longitude) and Y (latitude) directions.
 * 			Updates the current map tile coordinates and recalculates the corresponding longitude and latitude.
 *
 * @param dx Tile offset in X direction (east-west)
 * @param dy Tile offset in Y direction (north-south)
 */
void Maps::panMap(int8_t dx, int8_t dy)
{
    Maps::currentMapTile.tilex += dx;
    Maps::currentMapTile.tiley += dy;
    Maps::currentMapTile.lon = Maps::tilex2lon(Maps::currentMapTile.tilex, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = Maps::tiley2lat(Maps::currentMapTile.tiley, Maps::currentMapTile.zoom);
}

/**
 * @brief Center map on current GPS location
 *
 * @details Sets the map to follow the GPS and centers the map tile indices and coordinates on the current GPS location.
 *
 * @param lat GPS Latitude
 * @param lon GPS Longitude
 */
void Maps::centerOnGps(float lat, float lon)
{
    Maps::followGps = true;
    Maps::currentMapTile.tilex = Maps::lon2tilex(lon, Maps::currentMapTile.zoom);
    Maps::currentMapTile.tiley = Maps::lat2tiley(lat, Maps::currentMapTile.zoom);
    Maps::currentMapTile.lat = lat;
    Maps::currentMapTile.lon = lon;
}

/**
 * @brief Smooth scroll current map
 *
 * @details Smoothly scrolls the map view with inertia and friction, updating tile indices and offsets as needed.
 * 			Handles transitions when the scroll offset surpasses a threshold, triggering tile panning and preloading.
 *
 * @param dx Delta X input for scrolling
 * @param dy Delta Y input for scrolling
 */
void Maps::scrollMap(int16_t dx, int16_t dy)
{
    const float inertia = 0.5f;
    const float friction = 0.95f;
    const float maxSpeed = 10.0f;

    static float speedX = 0.0f, speedY = 0.0f;
    static unsigned long lastCacheStats = 0;

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
        Maps::preloadTiles(deltaTileX, deltaTileY);
        Maps::lastTileX = Maps::tileX;
        Maps::lastTileY = Maps::tileY;
    }
}

/**
 * @brief Preload Tiles for map scrolling
 *
 * @details Preloads map tiles in the direction of scrolling to enable smooth transitions.
 * 			Loads one or two tiles into a temporary sprite and uses it to update the main map buffer.
 *
 * @param dirX Direction to preload tiles in X (-1 for left, 1 for right, 0 for no horizontal scroll)
 * @param dirY Direction to preload tiles in Y (-1 for up, 1 for down, 0 for no vertical scroll)
 */
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

        // Calculate correct coordinates for the tile being preloaded
        // Use proper tile-to-coordinate conversion
        float tileLon = (tileToLoadX / (1 << Maps::zoomLevel)) * 360.0f - 180.0f;
        float tileLat = 90.0f - (tileToLoadY / (1 << Maps::zoomLevel)) * 180.0f;

        Maps::roundMapTile = Maps::getMapTile(
            tileLon,
            tileLat,
            Maps::zoomLevel,
            tileToLoadX,
            tileToLoadY
        );

        const int16_t offsetX = (dirX != 0) ? i * tileSize : 0;
        const int16_t offsetY = (dirY != 0) ? i * tileSize : 0;
        

        bool foundTile = false;
        
        // Try cache first for vector maps
        if (mapSet.vectorMap) 
        {
            foundTile = getCachedTile(Maps::roundMapTile.file, preloadSprite, offsetX, offsetY);
            if (foundTile) 
            {
                // Cache hit - no need to log every time
                // ESP_LOGI(TAG, "Tile found in cache: %s", Maps::roundMapTile.file);
            }
        }
        
        // If not in cache, try to load from file
        if (!foundTile)
        {
            if (mapSet.vectorMap)
            {
                // Create a temporary sprite for rendering to cache
                TFT_eSprite tempSprite = TFT_eSprite(&tft);
                tempSprite.createSprite(tileSize, tileSize);
                
                // Render tile to temporary sprite (this will cache it)
                foundTile = renderTile(Maps::roundMapTile.file, 0, 0, tempSprite);
                
                if (foundTile) 
                {
                    // Copy from temporary sprite to preload sprite
                    preloadSprite.pushImage(offsetX, offsetY, tileSize, tileSize, tempSprite.frameBuffer(0));
                }
                
                tempSprite.deleteSprite();
            } 
            else 
                foundTile = preloadSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);
        }

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

    preloadSprite.deleteSprite();
}

/**
 * @brief Loads an RGB332 color palette from a binary file.
 *
 * @details This function reads up to 256 bytes from the specified binary file and stores them in the global PALETTE array,
 *          setting the PALETTE_SIZE accordingly. The palette is expected to be in RGB332 format. 
 *
 * @param palettePath The path to the palette binary file.
 * @return true if the palette was loaded successfully and contains at least one color, false otherwise.
 */
bool Maps::loadPalette(const char* palettePath)
{
    FILE* f = fopen(palettePath, "rb");
    if (!f) 
        return false;
    
    // Read 4-byte header for number of colors 
    uint32_t numColors;
    if (fread(&numColors, 4, 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    // Read RGB888 colors (3 bytes per color) and convert to RGB332
    uint8_t rgb888[3];
    PALETTE_SIZE = 0;
    
    for (uint32_t i = 0; i < numColors && i < 256; i++) 
    {
        if (fread(rgb888, 3, 1, f) == 1) 
        {
            // Convert RGB332 to RGB888 
            uint8_t r332 = rgb888[0] & 0xE0;  // Keep top 3 bits
            uint8_t g332 = (rgb888[1] & 0xE0) >> 3;  // Keep top 3 bits, shift right
            uint8_t b332 = rgb888[2] >> 6;  // Keep top 2 bits
            PALETTE[i] = r332 | g332 | b332;
            PALETTE_SIZE++;
        }
    }
    
    fclose(f);
    ESP_LOGI(TAG, "Loaded palette: %u colors", PALETTE_SIZE);
    return PALETTE_SIZE > 0;
}

/**
 * @brief Converts a palette index to its corresponding RGB332 color value.
 *
 * @details descriptionLooks up the given index in the global PALETTE array and returns the corresponding RGB332 color.
 *          If the index is out of range (greater than or equal to PALETTE_SIZE), returns 0xFF (white) by default.
 *
 * @param idx The palette index to look up.
 * @return The RGB332 color value corresponding to the index, or 0xFF if the index is invalid.
 */
uint8_t Maps::paletteToRGB332(const uint32_t idx)
{
    if (idx < PALETTE_SIZE) 
        return PALETTE[idx];
    return 0xFF;
}

/**
 * @brief Darkens an RGB332 color by a specified amount.
 *
 * @details This function extracts the red, green, and blue components from the input RGB332 color value,
 *          multiplies each component by (1.0 - amount), and recombines them into a new RGB332 value. The default darkening amount is 0.4.
 *          This can be used to generate a visually darker shade of the original color for effects such as shading or selection highlighting.
 *
 * @param color The original RGB332 color value.
 * @param amount The fraction to darken each color component (default is 0.4).
 * @return The darkened RGB332 color value.
 */
uint8_t Maps::darkenRGB332(const uint8_t color, const float amount = 0.4f)
{
    uint8_t r = (color & 0xE0) >> 5;
    uint8_t g = (color & 0x1C) >> 2;
    uint8_t b = (color & 0x03);

    r = static_cast<uint8_t>(r * (1.0f - amount));
    g = static_cast<uint8_t>(g * (1.0f - amount));
    b = static_cast<uint8_t>(b * (1.0f - amount));

    return ((r << 5) | (g << 2) | b);
}

/**
 * @brief Converts an RGB332 color value to RGB565 format.
 *
 * @details This function extracts the red, green, and blue components from the 8-bit RGB332 input value, scales each component to match the RGB565 format, and recombines them into a 16-bit RGB565 value. 
 *
 * @param color The input color value in RGB332 format.
 * @return The corresponding color value in RGB565 format.
 */
uint16_t Maps::RGB332ToRGB565(const uint8_t color)
{
    // Convert RGB332 to RGB565 
    uint8_t r = (color & 0xE0);
    uint8_t g = (color & 0x1C) << 3;
    uint8_t b = (color & 0x03) << 6;
    
    // Convert to RGB565 format
    uint16_t r565 = (r >> 3) & 0x1F;  // 5 bits for red
    uint16_t g565 = (g >> 2) & 0x3F;  // 6 bits for green  
    uint16_t b565 = (b >> 3) & 0x1F;  // 5 bits for blue
    
    return (r565 << 11) | (g565 << 5) | b565;
}

/**
 * @brief Reads a variable-length integer (varint) from a byte array.
 *
 * @details This function decodes a 32-bit unsigned integer from the provided byte array starting at the given offset, using a variable-length encoding (varint). 
 *          It advances the offset as bytes are read, ensuring it does not exceed dataSize.
 *          The function returns the decoded value, or 0 if the offset moves past the end of the data buffer.
 *
 * @param data Pointer to the input byte array.
 * @param offset Reference to the current position in the byte array; will be updated to the new offset after reading.
 * @param dataSize The total size of the data buffer.
 * @return The decoded uint32_t varint value, or 0 if the offset exceeds dataSize.
 */
uint32_t Maps::readVarint(const uint8_t* data, size_t& offset, const size_t dataSize)
{
    uint32_t value = 0;
    uint8_t shift = 0;
    while (offset < dataSize && shift < 32) 
    {
        uint8_t byte = data[offset++];
        value |= ((uint32_t)(byte & 0x7F)) << shift;
        if ((byte & 0x80) == 0) 
            break;
        shift += 7;
    }
    if (offset > dataSize)
    {
        offset = dataSize;
        return 0;
    }
    return value;
}

/**
 * @brief Reads a zigzag-encoded integer from a byte array.
 *
 * @details This function decodes a 32-bit signed integer from the provided byte array starting at the given offset, using zigzag encoding. 
 *          It first reads a varint and then applies the zigzag decoding to convert it back to a signed integer. 
 *          The function advances the offset as bytes are read, ensuring it does not exceed dataSize.
 *          If the offset exceeds dataSize, it returns 0.
 *
 * @param data Pointer to the input byte array.
 * @param offset Reference to the current position in the byte array; will be updated to the new offset after reading.
 * @param dataSize The total size of the data buffer.
 * @return The decoded int32_t zigzag value, or 0 if the offset exceeds dataSize.
 */
int32_t Maps::readZigzag(const uint8_t* data, size_t& offset, const size_t dataSize)
{
    if (offset >= dataSize) 
        return 0;
    const uint32_t encoded = Maps::readVarint(data, offset, dataSize);
    return static_cast<int32_t>((encoded >> 1) ^ (-(int32_t)(encoded & 1)));
}

/**
 * @brief Converts a 16-bit unsigned integer in the range [0, 65535] to a pixel coordinate in the range [0, TILE_SIZE].
 *
 * @details This function scales the input value from the range of a 16-bit unsigned integer (0 to 65535) to a pixel coordinate within a tile of size TILE_SIZE (0 to 255). 
 *          It ensures that the resulting pixel coordinate is clamped within the valid range of [0, TILE_SIZE].
 *
 * @param val The input value in the range [0, 65535].
 * @return The corresponding pixel coordinate in the range [0, TILE_SIZE].
 */
int Maps::uint16ToPixel(const int32_t val)
{
    int p = static_cast<int>((val * TILE_SIZE_PLUS_ONE) / 65536);
    if (p < 0) 
        p = 0;
    if (p > TILE_SIZE)
        p = TILE_SIZE;
    return p;
}

/**
 * @brief Checks if a point lies on the margin of a tile.
 *
 * @details This function determines whether the given point (px, py) is located within the margin area of a tile, as defined by the MARGIN_PIXELS constant. 
 *          The margin is considered on any side of the tile (left, right, top, or bottom) if the point's coordinates are less than or equal to MARGIN_PIXELS,
 *          or greater than or equal to TILE_SIZE minus MARGIN_PIXELS.
 *
 * @param px The x-coordinate of the point.
 * @param py The y-coordinate of the point.
 * @return true if the point is on the margin, false otherwise.
 */
bool Maps::isPointOnMargin(const int px, const int py)
{
    return (px <= MARGIN_PIXELS || px >= TILE_SIZE - MARGIN_PIXELS || py <= MARGIN_PIXELS || py >= TILE_SIZE - MARGIN_PIXELS);
}

/** 
* @brief Checks if a value is near a target within a specified tolerance.
*
* @details This function determines whether the given value (val) is within a certain tolerance (tol) of the target value. 
*          The default tolerance is set to 2. It returns true if the absolute difference between val and target is less than or equal to tol.
*
* @param val The value to check.
* @param target The target value to compare against.
* @param tol The tolerance range (default is 2).
* @return true if val is within tol of target, false otherwise.
*/
bool Maps::isNear(int val, int target, int tol = 2) 
{
    return abs(val - target) <= tol;
}

/**
* @brief Determines if a line between two points should be drawn based on specific criteria.
*
* @details This function evaluates whether a line defined by two endpoints (px1, py1) and (px2, py2) should be drawn. 
*          It filters out lines that cross the entire tile from edge to edge (horizontally, vertically, or diagonally), 
*          lines that are excessively long (more than 1.5 times the diagonal of the tile), and lines where both endpoints are on the margin of the tile.
*          The function returns true if the line meets the criteria for drawing, and false otherwise.
*
* @param px1 The x-coordinate of the first endpoint.
* @param py1 The y-coordinate of the first endpoint.
* @param px2 The x-coordinate of the second endpoint.
* @param py2 The y-coordinate of the second endpoint.
* @return true if the line should be drawn, false otherwise.
*/
bool Maps::shouldDrawLine(const int px1, const int py1, const int px2, const int py2)
{
    if ((isNear(px1, 0) && isNear(px2, TILE_SIZE)) || (isNear(px1, TILE_SIZE) && isNear(px2, 0))) 
    {
        if ((isNear(py1, 0) && isNear(py2, TILE_SIZE)) || (isNear(py1, TILE_SIZE) && isNear(py2, 0)))
            return false;
        if (isNear(py1, py2)) 
            return false; 
    }
    if ((isNear(py1, 0) && isNear(py2, TILE_SIZE)) || (isNear(py1, TILE_SIZE) && isNear(py2, 0)))
    {
        if (isNear(px1, px2))
            return false; 
    }

    int dx = px2 - px1;
    int dy = py2 - py1;
    int len2 = dx*dx + dy*dy;
    if (len2 > (TILE_SIZE * TILE_SIZE * 3))
        return false;

    if (isPointOnMargin(px1, py1) && isPointOnMargin(px2, py2))
        return false;
    if ((px1 == px2) && (px1 <= MARGIN_PIXELS || px1 >= TILE_SIZE - MARGIN_PIXELS))
        return false;
    if ((py1 == py2) && (py1 <= MARGIN_PIXELS || py1 >= TILE_SIZE - MARGIN_PIXELS))
        return false;
    return true;
}

/**
 * @brief Fills a polygon on a sprite using the scanline algorithm.
 *
 * @details This function fills a polygon defined by the given vertex arrays (px, py) on the provided TFT_eSprite object (map) using the specified color (c). 
 *          It calculates the intersection points of the polygon with each scanline and draws horizontal lines between pairs of intersection points to fill the polygon.
 *          The function takes into account offsets for positioning within a larger context and ensures that drawing is constrained within the tile boundaries.
 *
 * @param map The TFT_eSprite object where the polygon will be drawn.
 * @param px Array of x-coordinates of the polygon vertices.
 * @param py Array of y-coordinates of the polygon vertices.
 * @param numPoints The number of vertices in the polygon.
 * @param c The color to fill the polygon with (in RGB565 format).
 * @param xOffset The x-offset to apply when drawing the polygon.
 * @param yOffset The y-offset to apply when drawing the polygon.
 */
void Maps::fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t color, const int xOffset, const int yOffset)
{
    static bool unifiedPoolLogged = false;
    
    int miny = py[0], maxy = py[0];
    for (int i = 1; i < numPoints; ++i) 
    {
        if (py[i] < miny) 
            miny = py[i];
        if (py[i] > maxy)
            maxy = py[i];
    }

    int *xints = nullptr;
    
    // Use RAII MemoryGuard for coordinate arrays
    MemoryGuard<int> xintsGuard(numPoints, 6); // Type 6 = coordArray
    xints = xintsGuard.get();
    if (!xints) 
    {
        ESP_LOGW(TAG, "fillPolygonGeneral: RAII MemoryGuard allocation failed");
        return;
    }
    if (!unifiedPoolLogged)
    {
        ESP_LOGI(TAG, "fillPolygonGeneral: Using RAII MemoryGuard for coordinate arrays");
        unifiedPoolLogged = true;
    }
    unifiedPoolHitCount++;
    
    if (!xints) 
        return;

    for (int y = miny; y <= maxy; ++y)
    {
        int nodes = 0;
        for (int i = 0, j = numPoints - 1; i < numPoints; j = i++) 
        {
            if ((py[i] < y && py[j] >= y) || (py[j] < y && py[i] >= y))
                xints[nodes++] = static_cast<int>(px[i] + (y - py[i]) * (px[j] - px[i]) / (py[j] - py[i]));
        }
        if (nodes > 1) 
        {
            std::sort(xints, xints + nodes);
            for (int i = 0; i < nodes; i += 2) 
            {
                if (i + 1 < nodes) 
                {
                    int x0 = xints[i] + xOffset;
                    int x1 = xints[i + 1] + xOffset;
                    int yy = y + yOffset;
                    if (yy >= 0 && yy <= TILE_SIZE + yOffset)
                    {
                        if (x0 < 0)
                            x0 = 0;
                        if (x1 > TILE_SIZE + xOffset)
                            x1 = TILE_SIZE + xOffset;
                        if (x1 > x0)
                            map.drawFastHLine(x0, yy, x1 - x0, color);
                    }
                }
            }
        }
    }
}

/**
* @brief Draws the border of a polygon on a sprite, with special handling for margin points.
*
* @details This function draws the border of a polygon defined by the given vertex arrays (px, py) on the provided TFT_eSprite object (map). 
*          It uses different colors for lines connecting margin points and non-margin points, and can optionally skip drawing lines between two margin points if fillPolygons is false.
*          The function takes into account offsets for positioning within a larger context and ensures that drawing is constrained within the tile boundaries.
*
* @param map The TFT_eSprite object where the polygon border will be drawn.
* @param px Array of x-coordinates of the polygon vertices.
* @param py Array of y-coordinates of the polygon vertices.
* @param numPoints The number of vertices in the polygon.
* @param borderColor The color to use for drawing the border lines (in RGB565 format).
* @param fillColor The color to use for lines between margin points (in RGB565 format).
* @param xOffset The x-offset to apply when drawing the polygon border.
* @param yOffset The y-offset to apply when drawing the polygon border.
*/
void Maps::drawPolygonBorder(TFT_eSprite &map, const int *px, const int *py, const int numPoints, const uint16_t borderColor, const uint16_t fillColor, const int xOffset, const int yOffset)
{
    if (numPoints < 2)
        return;

    // Cache first point margin for closing segment
    const bool marginFirst = isPointOnMargin(px[0], py[0]);
    bool marginA = marginFirst;

    for (uint32_t i = 0; i < numPoints - 1; ++i)
    {
        const bool marginB = isPointOnMargin(px[i+1], py[i+1]);
        const uint16_t color = (marginA && marginB) ? fillColor : borderColor;

        const int x0 = px[i] + xOffset;
        const int y0 = py[i] + yOffset;
        const int x1 = px[i+1] + xOffset;
        const int y1 = py[i+1] + yOffset;

        if (x0 >= 0 && x0 <= TILE_SIZE + xOffset && y0 >= 0 && y0 <= TILE_SIZE + yOffset &&
            x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset)
        {
            if (fillPolygons)
            {
                if (!(marginA && marginB))
                    map.drawLine(x0, y0, x1, y1, color);
                else
                {
                    map.drawLine(x0, y0, x1, y1, fillColor);
                    map.drawPixel(x0, y0, borderColor);
                    map.drawPixel(x1, y1, borderColor);
                }
            }
            else if (!(marginA && marginB))
                map.drawLine(x0, y0, x1, y1, borderColor);
            else
            {
                map.drawLine(x0, y0, x1, y1, TFT_WHITE);
                map.drawPixel(x0, y0, borderColor);
                map.drawPixel(x1, y1, borderColor);
            }
        }
        marginA = marginB;  // Reuse for next iteration
    }
    // Closing segment: last point to first point
    const bool marginB = marginFirst;  // Reuse cached first point
    const uint16_t color = (marginA && marginB) ? fillColor : borderColor;
    const int x0 = px[numPoints-1] + xOffset;
    const int y0 = py[numPoints-1] + yOffset;
    const int x1 = px[0] + xOffset;
    const int y1 = py[0] + yOffset;

    if (x0 >= 0 && x0 <= TILE_SIZE + xOffset && y0 >= 0 && y0 <= TILE_SIZE + yOffset &&
        x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset)
    {
        if (fillPolygons)
        {
            if (!(marginA && marginB))
                map.drawLine(x0, y0, x1, y1, color);
            else
            {
                map.drawLine(x0, y0, x1, y1, fillColor);
                map.drawPixel(x0, y0, borderColor);
                map.drawPixel(x1, y1, borderColor);
            }
        }
        else if (!(marginA && marginB))
            map.drawLine(x0, y0, x1, y1, borderColor);
        else
        {
            map.drawLine(x0, y0, x1, y1, TFT_WHITE);
            map.drawPixel(x0, y0, borderColor);
            map.drawPixel(x1, y1, borderColor);
        }
    }
}   

/**
 * @brief Initialize tile cache system
 *
 * @details Initializes the tile cache system by clearing existing cache, reserving space for the maximum number of cached tiles,
 *          resetting the access counter, and logging the cache capacity for debugging purposes.
 */
void Maps::initTileCache()
{
    tileCache.clear();
    tileCache.reserve(maxCachedTiles);
    cacheAccessCounter = 0;
    ESP_LOGI(TAG, "Tile cache initialized with %zu tiles capacity", maxCachedTiles);
}


/**
 * @brief Calculate hash for tile identification
 *
 * @details Creates a simple hash from the file path for cache lookup.
 *
 * @param filePath The file path to hash.
 * @return Hash value for the file path.
 */
uint32_t Maps::calculateTileHash(const char* filePath)
{
    uint32_t hash = 0;
    const char* p = filePath;
    while (*p) 
    {
        hash = hash * 31 + *p;
        p++;
    }
    return hash;
}

/**
 * @brief Get tile from cache
 *
 * @details Attempts to retrieve a tile from the cache. If found, copies it to the target sprite.
 *
 * @param filePath The file path of the tile to retrieve.
 * @param target The target sprite to copy the cached tile to.
 * @param xOffset The x-offset for positioning.
 * @param yOffset The y-offset for positioning.
 * @return true if tile was found in cache, false otherwise.
 */
bool Maps::getCachedTile(const char* filePath, TFT_eSprite& target, int16_t xOffset, int16_t yOffset)
{
    if (maxCachedTiles == 0) return false; // Cache disabled
    
    uint32_t tileHash = calculateTileHash(filePath);
    
    for (auto& cachedTile : tileCache) 
    {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) 
        {
            // Found in cache - update access time and copy sprite
            cachedTile.lastAccess = ++cacheAccessCounter;
            
            // Copy the cached sprite to target position
            target.pushImage(xOffset, yOffset, tileWidth, tileHeight, cachedTile.sprite->frameBuffer(0));
            return true;
        }
    }
    
    return false; // Not found in cache
}

/**
 * @brief Add rendered tile to cache
 *
 * @details Adds a newly rendered tile to the cache, evicting LRU entries if necessary.
 *
 * @param filePath The file path of the tile.
 * @param source The source sprite containing the rendered tile.
 */
void Maps::addToCache(const char* filePath, TFT_eSprite& source)
{
    if (maxCachedTiles == 0) return; // Cache disabled
    
    uint32_t tileHash = calculateTileHash(filePath);
    
    // Check if already in cache
    for (auto& cachedTile : tileCache) 
    {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) 
        {
            cachedTile.lastAccess = ++cacheAccessCounter;
            return; // Already cached
        }
    }
    
    // Need to add new entry
    if (tileCache.size() >= maxCachedTiles) 
        evictLRUTile(); // Make room
    
    // Create new cache entry
    CachedTile newEntry;
    newEntry.sprite = new TFT_eSprite(&tft);
    newEntry.sprite->createSprite(tileWidth, tileHeight);
    
    // Copy the rendered tile to cache
    newEntry.sprite->pushImage(0, 0, tileWidth, tileHeight, source.frameBuffer(0));
    
    newEntry.tileHash = tileHash;
    newEntry.lastAccess = ++cacheAccessCounter;
    newEntry.isValid = true;
    strncpy(newEntry.filePath, filePath, sizeof(newEntry.filePath) - 1);
    newEntry.filePath[sizeof(newEntry.filePath) - 1] = '\0';
    
    tileCache.push_back(newEntry);
}

/**
 * @brief Remove least recently used tile from cache
 *
 * @details Finds and removes the tile with the oldest access time.
 */
void Maps::evictLRUTile()
{
    if (tileCache.empty()) return;
    
    // Intelligent LRU eviction - consider both access time and memory usage
    auto lruIt = tileCache.begin();
    uint32_t bestScore = lruIt->lastAccess;
    size_t bestMemoryUsage = 0;
    
    // Calculate memory usage for each tile
    for (auto it = tileCache.begin(); it != tileCache.end(); ++it) 
    {
        size_t tileMemory = (it->sprite) ? (tileWidth * tileHeight * 2) : 0; // 2 bytes per pixel
        
        // Score based on access time and memory usage (lower is better)
        uint32_t score = it->lastAccess + (tileMemory / 1024); // Add memory penalty
        
        if (score < bestScore || (score == bestScore && tileMemory > bestMemoryUsage))
        {
            bestScore = score;
            bestMemoryUsage = tileMemory;
            lruIt = it;
        }
    }
    
    // Free the sprite memory
    if (lruIt->sprite) 
    {
        lruIt->sprite->deleteSprite();
        delete lruIt->sprite;
    }
    
    // Remove from cache
    tileCache.erase(lruIt);
}

/**
 * @brief Clear all cached tiles
 *
 * @details Frees all cached tiles and clears the cache.
 */
void Maps::clearTileCache()
{
    for (auto& cachedTile : tileCache)
    {
        if (cachedTile.sprite) 
        {
            cachedTile.sprite->deleteSprite();
            delete cachedTile.sprite;
        }
    }
    tileCache.clear();
    cacheAccessCounter = 0;
}

/**
 * @brief Get current cache memory usage
 *
 * @details Calculates the current memory usage of the tile cache.
 *
 * @return Memory usage in bytes.
 */
size_t Maps::getCacheMemoryUsage()
{
    size_t memoryUsage = 0;
    for (const auto& cachedTile : tileCache)
    {
        if (cachedTile.isValid && cachedTile.sprite) 
            memoryUsage += tileWidth * tileHeight * 2; // RGB565 = 2 bytes per pixel
    }
    return memoryUsage;
}

/**
* @brief Renders a map tile from a binary file onto a sprite.
*
* @details This function reads a binary map tile file, decodes its drawing commands, and renders the resulting graphics onto the provided TFT_eSprite object (map). 
*          It supports line drawing and polygon filling based on the commands in the file, using a predefined color palette. The function handles batching of line drawing for performance optimization.
*          The rendered tile is positioned on the sprite using the specified xOffset and yOffset.
*
* @param path The file path to the binary map tile.
* @param xOffset The x-offset to apply when rendering the tile on the sprite.
* @param yOffset The y-offset to apply when rendering the tile on the sprite.
* @param map The TFT_eSprite object where the tile will be rendered.
* @return true if the tile was rendered successfully, false otherwise.
*/
bool Maps::renderTile(const char* path, const int16_t xOffset, const int16_t yOffset, TFT_eSprite &map)
{
    static bool isPaletteLoaded = false;
    static bool unifiedPoolLogged = false;
    static bool unifiedPolylineLogged = false;
    static bool unifiedLineBatchLogged = false;
    if (!isPaletteLoaded) 
        isPaletteLoaded = Maps::loadPalette("/sdcard/VECTMAP/palette.bin");

    if (!path || path[0] == '\0')
        return false;

    // Try to get tile from cache first
    if (getCachedTile(path, map, xOffset, yOffset))
        return true; // Tile found in cache

    // Direct file read
    FILE* file = fopen(path, "rb");
    if (!file)
       return false;
    
    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Use RAII MemoryGuard for data allocation
    MemoryGuard<uint8_t> dataGuard(fileSize, 0); // Type 0 = general
    uint8_t* data = dataGuard.get();
    if (!data)
    {
        fclose(file);
        return false;
    }
    if (!unifiedPoolLogged)
        unifiedPoolLogged = true;

    const size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) 
        return false;

    // Update memory statistics (simplified)
    currentMemoryUsage = esp_get_free_heap_size();
    if (currentMemoryUsage > peakMemoryUsage) 
        peakMemoryUsage = currentMemoryUsage;

    size_t offset = 0;
    const size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = RGB332ToRGB565(current_color);

    const uint32_t num_cmds = readVarint(data, offset, dataSize);
    if (num_cmds == 0)
        return false;

    // Enable DMA for faster rendering
    map.initDMA();

    // Track memory allocations for statistics
    totalMemoryAllocations++;

    int executed = 0;
    unsigned long renderStart = millis_idf();

    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) 
    {
        if (offset >= dataSize) 
            break;
        const size_t cmdStartOffset = offset;
        const uint32_t cmdType = readVarint(data, offset, dataSize);

        switch (cmdType)
        {
            case SET_COLOR:
                if (offset < dataSize) 
                {
                    current_color = data[offset++];
                    currentDrawColor = RGB332ToRGB565(current_color);
                    executed++;
                }
                break;
            case SET_COLOR_INDEX:
                {
                    const uint32_t color_index = readVarint(data, offset, dataSize);
                    current_color = paletteToRGB332(color_index);
                    currentDrawColor = RGB332ToRGB565(current_color);
                    executed++;
                }
                break;
            case DRAW_POLYLINE:
                {
                    const uint32_t lineWidth = readVarint(data, offset, dataSize);
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 2)
                    {
                        MemoryGuard<int> pxGuard(numPoints, 6);
                        MemoryGuard<int> pyGuard(numPoints, 6);
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py)
                        {
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                            {
                                readZigzag(data, offset, dataSize);
                                readZigzag(data, offset, dataSize);
                            }
                            continue;
                        }
                        if (!unifiedPolylineLogged)
                            unifiedPolylineLogged = true;
                        unifiedPoolHitCount += 2;

                        int32_t prevX = readZigzag(data, offset, dataSize);
                        int32_t prevY = readZigzag(data, offset, dataSize);
                        px[0] = uint16ToPixel(prevX) + xOffset;
                        py[0] = uint16ToPixel(prevY) + yOffset;
                        for (uint32_t i = 1; i < numPoints; ++i)
                        {
                            const int32_t deltaX = readZigzag(data, offset, dataSize);
                            const int32_t deltaY = readZigzag(data, offset, dataSize);
                            prevX += deltaX;
                            prevY += deltaY;
                            px[i] = uint16ToPixel(prevX) + xOffset;
                            py[i] = uint16ToPixel(prevY) + yOffset;
                        }
                        for (uint32_t i = 1; i < numPoints; ++i)
                        {
                            if (shouldDrawLine(px[i-1] - xOffset, py[i-1] - yOffset, px[i] - xOffset, py[i] - yOffset))
                                map.drawWideLine(px[i-1], py[i-1], px[i], py[i], lineWidth, currentDrawColor);
                        }
                        executed++;
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                        {
                            readZigzag(data, offset, dataSize);
                            readZigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case DRAW_STROKE_POLYGON:
                {
                    readVarint(data, offset, dataSize);
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 3)
                    {
                        MemoryGuard<int> pxGuard(numPoints, 6);
                        MemoryGuard<int> pyGuard(numPoints, 6);
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py)
                        {
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                            {
                                readZigzag(data, offset, dataSize);
                                readZigzag(data, offset, dataSize);
                            }
                            break;
                        }

                        const int32_t firstX = readZigzag(data, offset, dataSize);
                        const int32_t firstY = readZigzag(data, offset, dataSize);
                        px[0] = uint16ToPixel(firstX);
                        py[0] = uint16ToPixel(firstY);
                        int prevX = firstX;
                        int prevY = firstY;
                        for (uint32_t i = 1; i < numPoints; ++i)
                        {
                            const int32_t deltaX = readZigzag(data, offset, dataSize);
                            const int32_t deltaY = readZigzag(data, offset, dataSize);
                            prevX += deltaX;
                            prevY += deltaY;
                            px[i] = uint16ToPixel(prevX);
                            py[i] = uint16ToPixel(prevY);
                        }
                        if (fillPolygons && numPoints >= 3)
                            fillPolygonGeneral(map, px, py, numPoints, currentDrawColor, xOffset, yOffset);

                        const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                        drawPolygonBorder(map, px, py, numPoints, borderColor, currentDrawColor, xOffset, yOffset);
                        executed++;
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                        {
                            readZigzag(data, offset, dataSize);
                            readZigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            default:
                if (offset < dataSize - 4)
                    offset += 4;
                break;
        }

        if (offset <= cmdStartOffset)
            break;
    }

    if (executed == 0) 
        return false;

    // Add successfully rendered tile to cache
    addToCache(path, map);

    unsigned long renderTime = millis_idf() - renderStart;

    return true;
}

/**
 * @brief Initialize the unified memory pool for efficient memory management
 * 
 * @details Sets up a unified memory pool system with mutex protection, calculates optimal pool size based on available PSRAM or RAM,
 *          reserves space for pool entries, and initializes hit/miss counters for performance monitoring.
 */
void Maps::initUnifiedPool()
{
    ESP_LOGI(TAG, "Initializing unified memory pool...");
    
    if (unifiedPoolMutex == nullptr)
    {
        unifiedPoolMutex = xSemaphoreCreateMutex();
        if (unifiedPoolMutex == nullptr) 
        {
            ESP_LOGE(TAG, "Failed to create unified pool mutex");
            return;
        }
    }
    
    #ifdef BOARD_HAS_PSRAM
        size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        maxUnifiedPoolEntries = std::min(static_cast<size_t>(100), psramFree / (1024 * 32)); // 32KB per entry
        ESP_LOGI(TAG, "PSRAM available: %zu bytes, setting unified pool size to %zu entries", psramFree, maxUnifiedPoolEntries);
    #else
        size_t ramFree = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        maxUnifiedPoolEntries = std::min(static_cast<size_t>(25), ramFree / (1024 * 64)); // 64KB per entry
        ESP_LOGI(TAG, "RAM available: %zu bytes, setting unified pool size to %zu entries", ramFree, maxUnifiedPoolEntries);
    #endif
    
    unifiedPool.clear();
    unifiedPool.reserve(maxUnifiedPoolEntries);
    unifiedPoolHitCount = 0;
    unifiedPoolMissCount = 0;
    
    ESP_LOGI(TAG, "Unified memory pool initialized");
}

/**
 * @brief Prefetch adjacent tiles for faster loading
 * 
 * @details Preloads tiles around the current position to reduce loading time during scrolling.
 *          Uses background task to avoid blocking the main rendering thread.
 * 
 * @param centerX Current tile X coordinate
 * @param centerY Current tile Y coordinate  
 * @param zoom Current zoom level
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



/**
 * @brief Implement a unified memory allocation function that uses a memory pool
 *
 * @details Attempts to allocate memory from the unified pool first, falling back to direct heap allocation if pool is full.
 *          Uses mutex protection for thread safety and tracks allocation statistics for performance monitoring.
 *
 * @param size Size of memory to allocate in bytes
 * @param type Type of allocation for tracking purposes (0=general, 6=coordArray, etc.)
 * @return void* Pointer to allocated memory, or nullptr if allocation failed
 */
void* Maps::unifiedAlloc(size_t size, uint8_t type)
{
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE)
    {
        for (auto& entry : unifiedPool) 
        {
            if (!entry.isInUse && entry.size >= size) 
            {
                entry.isInUse = true;
                entry.allocationCount++;
                entry.type = type;
                unifiedPoolHitCount++;
                xSemaphoreGive(unifiedPoolMutex);
                return entry.ptr;
            }
        }
        
        if (unifiedPool.size() < maxUnifiedPoolEntries) 
        {
            void* ptr = nullptr;
            #ifdef BOARD_HAS_PSRAM
                ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            #else
                ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);
            #endif
            
            if (ptr) 
            {
                UnifiedPoolEntry entry;
                entry.ptr = ptr;
                entry.size = size;
                entry.isInUse = true;
                entry.allocationCount = 1;
                entry.type = type;
                unifiedPool.push_back(entry);
                unifiedPoolHitCount++;
                xSemaphoreGive(unifiedPoolMutex);
                return ptr;
            }
        }
        
        unifiedPoolMissCount++;
        xSemaphoreGive(unifiedPoolMutex);
    }
    
    unifiedPoolMissCount++;
    #ifdef BOARD_HAS_PSRAM
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    #else
        return heap_caps_malloc(size, MALLOC_CAP_8BIT);
    #endif
}

/**
 * @brief Deallocate memory allocated from the unified memory pool
 *
 * @details Marks memory as available in the unified pool if it was allocated from there, otherwise calls standard free.
 *          Uses mutex protection for thread safety and handles null pointer gracefully.
 * 
 * @param ptr Pointer to memory to deallocate
 */
void Maps::unifiedDealloc(void* ptr)
{
    if (!ptr) return;
    
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) 
    {
        for (auto& entry : unifiedPool) 
        {
            if (entry.ptr == ptr && entry.isInUse)
            {
                entry.isInUse = false;
                xSemaphoreGive(unifiedPoolMutex);
                return;
            }
        }
        xSemaphoreGive(unifiedPoolMutex);
    }
    
    heap_caps_free(ptr);
}

