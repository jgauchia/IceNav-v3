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
#include <cmath>
#include <climits>
#include <cstdint>
#include <sys/stat.h>
#include "esp_timer.h"
#include "esp_system.h"

static inline uint32_t millis_idf() { return (uint32_t)(esp_timer_get_time() / 1000); }

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData; /**< Vector containing track waypoints */
const char* TAG = "Maps";


uint16_t Maps::currentDrawColor = TFT_WHITE;

// Tile cache system static variables
std::vector<Maps::CachedTile> Maps::tileCache;
size_t Maps::maxCachedTiles = 0;
uint32_t Maps::cacheAccessCounter = 0;

// Background prefetch system static variables (multi-core)
QueueHandle_t Maps::prefetchQueue = nullptr;
TaskHandle_t Maps::prefetchTaskHandle = nullptr;
SemaphoreHandle_t Maps::prefetchMutex = nullptr;
TFT_eSprite* Maps::prefetchRenderSprite = nullptr;
volatile bool Maps::prefetchTaskRunning = false;

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
Maps::Maps() : fillPolygons(true),
               fgbLastLat_(0), fgbLastLon_(0), fgbLastZoom_(0), fgbNeedsRender_(true)
{
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

    // Pre-allocate scroll sprites (avoid allocation during scroll)
    Maps::preloadSprite.deleteSprite();
    Maps::preloadSprite.createSprite(mapTileSize * 2, mapTileSize * 2);
    Maps::tileRenderSprite.deleteSprite();
    Maps::tileRenderSprite.createSprite(mapTileSize, mapTileSize);

    Maps::oldMapTile = {};     // Old Map tile coordinates and zoom
    Maps::currentMapTile = {}; // Current Map tile coordinates and zoom
    Maps::roundMapTile = {};    // Boundaries Map tiles
    Maps::navArrowPosition = {0, 0};              // Map Arrow position

    Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};
    
    // Initialize tile cache system
    initTileCache();

    // Initialize background prefetch system (multi-core)
    initPrefetchSystem();

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
    Maps::preloadSprite.deleteSprite();
    Maps::tileRenderSprite.deleteSprite();

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

    const float lat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
    const float lon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

    // FlatGeobuf rendering path
    if (mapSet.vectorMap)
    {
        // Check if position changed enough to require re-render
        // Use ~10m threshold (approx 0.0001 degrees)
        constexpr float POS_THRESHOLD = 0.0001f;
        bool posChanged = fabsf(lat - fgbLastLat_) > POS_THRESHOLD ||
                          fabsf(lon - fgbLastLon_) > POS_THRESHOLD ||
                          zoom != fgbLastZoom_;

        if (posChanged || fgbNeedsRender_)
        {
            Maps::isMapFound = renderFgbViewport(lat, lon, zoom, Maps::mapTempSprite);
            if (!Maps::isMapFound)
            {
                ESP_LOGW(TAG, "FGB: No map data found");
                Maps::mapTempSprite.fillScreen(TFT_BLACK);
                Maps::showNoMap(Maps::mapTempSprite);
            }
            // Update cache
            fgbLastLat_ = lat;
            fgbLastLon_ = lon;
            fgbLastZoom_ = zoom;
            fgbNeedsRender_ = false;
        }
        return;
    }

    // Legacy PNG tile rendering path
    bool foundRoundMap = false;
    bool missingMap = false;

    Maps::currentMapTile = Maps::getMapTile(lon, lat, Maps::zoomLevel, 0, 0);

    // Detects if tile changes from actual GPS position
    if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 ||
        Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
        Maps::currentMapTile.tilex != Maps::oldMapTile.tilex ||
        Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
    {
        const int16_t size = Maps::mapTileSize;

        Maps::mapTempSprite.fillScreen(TFT_WHITE);

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
    // Base physics parameters
    const float baseFriction = 0.92f;    // Friction when finger lifted
    const float maxSpeed = 25.0f;        // Absolute max speed limit
    const float smoothingFactor = 0.3f;  // Smooth finger speed changes (0-1)

    static float speedX = 0.0f, speedY = 0.0f;
    static float smoothedFingerSpeed = 0.0f;  // Smoothed finger velocity
    static bool prefetchTriggeredX = false;
    static bool prefetchTriggeredY = false;
    static int8_t lastPrefetchDirX = 0;
    static int8_t lastPrefetchDirY = 0;

    // Detect finger velocity from delta
    const float instantFingerSpeed = sqrtf(dx * dx + dy * dy);

    // Smooth the finger speed to handle acceleration/deceleration
    smoothedFingerSpeed = smoothedFingerSpeed * (1.0f - smoothingFactor) +
                          instantFingerSpeed * smoothingFactor;

    const bool fingerActive = (instantFingerSpeed > 0.5f);

    // Dynamic responsiveness based on smoothed finger speed
    // Slow movement: more precise (0.7), Fast movement: more responsive (1.0)
    const float responsiveness = fingerActive ?
        fminf(1.0f, 0.7f + smoothedFingerSpeed * 0.025f) : 0.0f;

    // Less friction while finger is active, more when released (inertia)
    const float friction = fingerActive ? 0.6f : baseFriction;

    // Physics: velocity = previous * friction + input * responsiveness
    speedX = speedX * friction + dx * responsiveness;
    speedY = speedY * friction + dy * responsiveness;

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
    const int16_t prefetchThreshold = threshold / 2;  // Anticipate prefetch at 50% of threshold

    // Detect scroll direction and trigger anticipatory prefetch
    int8_t dirX = (speedX > 0.5f) ? 1 : (speedX < -0.5f) ? -1 : 0;
    int8_t dirY = (speedY > 0.5f) ? 1 : (speedY < -0.5f) ? -1 : 0;

    // Reset prefetch trigger if direction changed
    if (dirX != lastPrefetchDirX) { prefetchTriggeredX = false; lastPrefetchDirX = dirX; }
    if (dirY != lastPrefetchDirY) { prefetchTriggeredY = false; lastPrefetchDirY = dirY; }

    // Anticipatory prefetch: trigger when approaching threshold (before tile change)
    if (!prefetchTriggeredX && dirX != 0 && abs(Maps::offsetX) > prefetchThreshold)
    {
        enqueueSurroundingTiles(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel, dirX, 0);
        prefetchTriggeredX = true;
    }
    if (!prefetchTriggeredY && dirY != 0 && abs(Maps::offsetY) > prefetchThreshold)
    {
        enqueueSurroundingTiles(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel, 0, dirY);
        prefetchTriggeredY = true;
    }

    if (Maps::offsetX <= -threshold)
    {
        Maps::tileX--;
        Maps::offsetX += tileSize;
        Maps::scrollUpdated = true;
        prefetchTriggeredX = false;  // Reset for next tile
    }
    else if (Maps::offsetX >= threshold)
    {
        Maps::tileX++;
        Maps::offsetX -= tileSize;
        Maps::scrollUpdated = true;
        prefetchTriggeredX = false;  // Reset for next tile
    }

    if (Maps::offsetY <= -threshold)
    {
        Maps::tileY--;
        Maps::offsetY += tileSize;
        Maps::scrollUpdated = true;
        prefetchTriggeredY = false;  // Reset for next tile
    }
    else if (Maps::offsetY >= threshold)
    {
        Maps::tileY++;
        Maps::offsetY -= tileSize;
        Maps::scrollUpdated = true;
        prefetchTriggeredY = false;  // Reset for next tile
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

    // Ensure sprites are created (lazy init if needed)
    if (!preloadSprite.getBuffer())
        preloadSprite.createSprite(mapTileSize * 2, mapTileSize * 2);
    if (!tileRenderSprite.getBuffer())
        tileRenderSprite.createSprite(mapTileSize, mapTileSize);

    const int16_t startX = tileX + dirX;
    const int16_t startY = tileY + dirY;

    for (int8_t i = 0; i < 2; ++i)
    {
        const int16_t tileToLoadX = startX + ((dirX == 0) ? i - 1 : 0);
        const int16_t tileToLoadY = startY + ((dirY == 0) ? i - 1 : 0);

        // Calculate correct coordinates for the tile being preloaded
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

        // Try to load tile from file (PNG only - FGB uses full viewport re-render)
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
 * @brief Darkens an RGB565 color by a specified amount.
 *
 * @details This function extracts the red, green, and blue components from the input RGB565 color value,
 *          multiplies each component by (1.0 - amount), and recombines them into a new RGB565 value.
 *
 * @param color The original RGB565 color value.
 * @param amount The fraction to darken each color component (default is 0.4).
 * @return The darkened RGB565 color value.
 */
uint16_t Maps::darkenRGB565(const uint16_t color, const float amount = 0.4f)
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
        return;

    unifiedPoolHitCount++;

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
                    if (yy >= 0 && yy < tileHeight + yOffset)
                    {
                        if (x0 < 0)
                            x0 = 0;
                        if (x1 > tileWidth + xOffset)
                            x1 = tileWidth + xOffset;
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

    // Calculate cache size based on available PSRAM
    // Each cached tile uses mapTileSize * mapTileSize * 2 bytes (RGB565)
    const size_t tileSizeBytes = mapTileSize * mapTileSize * 2;  // 256*256*2 = 128KB per tile
    #ifdef BOARD_HAS_PSRAM
        // Fixed cache size: 3 tiles (1 row/column for prefetch)
        maxCachedTiles = 3;
    #else
        // Without PSRAM, disable cache
        maxCachedTiles = 0;
    #endif

    tileCache.reserve(maxCachedTiles);
    cacheAccessCounter = 0;
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
            // Found in cache - update access time
            cachedTile.lastAccess = ++cacheAccessCounter;

            // Direct memory copy from cache sprite to target (row by row)
            // This avoids byte-order issues with pushSprite/pushImage methods
            uint16_t* srcBuffer = (uint16_t*)cachedTile.sprite->getBuffer();
            uint16_t* dstBuffer = (uint16_t*)target.getBuffer();
            int dstWidth = target.width();

            // Validate bounds
            if (xOffset >= 0 && yOffset >= 0 &&
                xOffset + mapTileSize <= dstWidth &&
                yOffset + mapTileSize <= target.height())
            {
                for (int y = 0; y < mapTileSize; y++)
                {
                    int srcOffset = y * mapTileSize;
                    int dstOffset = (yOffset + y) * dstWidth + xOffset;
                    memcpy(&dstBuffer[dstOffset], &srcBuffer[srcOffset], mapTileSize * sizeof(uint16_t));
                }
            }

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
 * @param srcX X offset in source sprite where tile data starts.
 * @param srcY Y offset in source sprite where tile data starts.
 */
void Maps::addToCache(const char* filePath, TFT_eSprite& source, int16_t srcX, int16_t srcY)
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

    // Create new cache entry with 16-bit color depth
    CachedTile newEntry;
    newEntry.sprite = new TFT_eSprite(&tft);
    newEntry.sprite->setColorDepth(16);
    void* buffer = newEntry.sprite->createSprite(mapTileSize, mapTileSize);

    if (!buffer)
    {
        ESP_LOGE(TAG, "Failed to create cache sprite");
        delete newEntry.sprite;
        return;
    }

    // Direct memory copy from source region to cache sprite (row by row)
    // This avoids byte-order issues with pushSprite/pushImage methods
    uint16_t* srcBuffer = (uint16_t*)source.getBuffer();
    uint16_t* dstBuffer = (uint16_t*)newEntry.sprite->getBuffer();
    int srcWidth = source.width();

    // Validate bounds
    if (srcX >= 0 && srcY >= 0 &&
        srcX + mapTileSize <= srcWidth &&
        srcY + mapTileSize <= source.height())
    {
        for (int y = 0; y < mapTileSize; y++)
        {
            int srcOffset = (srcY + y) * srcWidth + srcX;
            int dstOffset = y * mapTileSize;
            memcpy(&dstBuffer[dstOffset], &srcBuffer[srcOffset], mapTileSize * sizeof(uint16_t));
        }
    }

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
    // FGB uses full viewport re-render, no tile prefetch needed
    if (mapSet.vectorMap)
        return;

    // PNG: Prefetch 3x3 grid around current tile (just pre-read to FS buffer)
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

            // Pre-read file to FS buffer
            FILE* pngFile = fopen(prefetchTile.file, "rb");
            if (pngFile)
            {
                char buffer[4096];
                while (fread(buffer, 1, sizeof(buffer), pngFile) > 0) {}
                fclose(pngFile);
            }
        }
    }
}

/**
 * @brief Background prefetch task that runs on Core 0
 *
 * @details Continuously processes the prefetch queue, loading tiles into cache in the background.
 *          Runs at low priority to not interfere with GPS task on the same core.
 *
 * @param pvParameters Maps instance pointer for accessing renderTile
 */
void Maps::prefetchTask(void* pvParameters)
{
    Maps* mapsInstance = static_cast<Maps*>(pvParameters);
    PrefetchRequest request;

    while (prefetchTaskRunning)
    {
        // Wait for a request with timeout (allows checking prefetchTaskRunning flag)
        if (xQueueReceive(prefetchQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            ESP_LOGD(TAG, "Task received request: %s", request.filePath);
            // Check if already in cache
            if (prefetchMutex && xSemaphoreTake(prefetchMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                // Quick check if file exists
                FILE* testFile = fopen(request.filePath, "rb");
                if (testFile)
                {
                    fclose(testFile);

                    // Check if already cached using hash
                    uint32_t tileHash = 0;
                    const char* p = request.filePath;
                    while (*p)
                    {
                        tileHash = tileHash * 31 + *p;
                        p++;
                    }

                    bool alreadyCached = false;
                    for (const auto& cachedTile : tileCache)
                    {
                        if (cachedTile.isValid && cachedTile.tileHash == tileHash)
                        {
                            alreadyCached = true;
                            break;
                        }
                    }

                    if (!alreadyCached)
                    {
                        // PNG: just pre-read file to FS buffer (no cache)
                        // Buffer 4KB max - task stack is 8KB, can't use larger buffers
                        FILE* pngFile = fopen(request.filePath, "rb");
                        if (pngFile)
                        {
                            char buffer[4096];
                            while (fread(buffer, 1, sizeof(buffer), pngFile) > 0) {}
                            fclose(pngFile);
                        }
                    }
                }

                xSemaphoreGive(prefetchMutex);
            }

            // Yield to allow other tasks to run
            taskYIELD();
        }
    }

    vTaskDelete(NULL);
}

/**
 * @brief Initialize the background prefetch system
 *
 * @details Creates the prefetch queue, mutex, render sprite, and starts the prefetch task on Core 0.
 */
void Maps::initPrefetchSystem()
{
    if (prefetchTaskRunning)
        return;

    // Create queue for prefetch requests (hold up to 16 requests)
    prefetchQueue = xQueueCreate(16, sizeof(PrefetchRequest));
    if (!prefetchQueue)
    {
        ESP_LOGE(TAG, "Failed to create prefetch queue");
        return;
    }

    // Create mutex for sprite access
    prefetchMutex = xSemaphoreCreateMutex();
    if (!prefetchMutex)
    {
        ESP_LOGE(TAG, "Failed to create prefetch mutex");
        vQueueDelete(prefetchQueue);
        prefetchQueue = nullptr;
        return;
    }

    // Create render sprite for background rendering
    prefetchRenderSprite = new TFT_eSprite(&tft);
    prefetchRenderSprite->setColorDepth(16);
    if (!prefetchRenderSprite->createSprite(mapTileSize, mapTileSize))
    {
        ESP_LOGE(TAG, "Failed to create prefetch render sprite");
        vSemaphoreDelete(prefetchMutex);
        prefetchMutex = nullptr;
        vQueueDelete(prefetchQueue);
        prefetchQueue = nullptr;
        delete prefetchRenderSprite;
        prefetchRenderSprite = nullptr;
        return;
    }

    // Start prefetch task on Core 0 (same as GPS, low priority)
    prefetchTaskRunning = true;
    BaseType_t result = xTaskCreatePinnedToCore(
        prefetchTask,           // Task function
        "PrefetchTask",         // Task name
        8192,                   // Stack size (8KB)
        this,                   // Pass Maps instance for renderTile access
        2,                      // Priority (higher for faster prefetch)
        &prefetchTaskHandle,    // Task handle
        0                       // Core 0
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create prefetch task");
        prefetchTaskRunning = false;
        prefetchRenderSprite->deleteSprite();
        delete prefetchRenderSprite;
        prefetchRenderSprite = nullptr;
        vSemaphoreDelete(prefetchMutex);
        prefetchMutex = nullptr;
        vQueueDelete(prefetchQueue);
        prefetchQueue = nullptr;
        return;
    }

}

/**
 * @brief Stop the background prefetch system
 *
 * @details Signals the prefetch task to stop and cleans up resources.
 */
void Maps::stopPrefetchSystem()
{
    if (!prefetchTaskRunning)
        return;


    prefetchTaskRunning = false;

    // Wait for task to finish (with timeout)
    vTaskDelay(pdMS_TO_TICKS(200));

    // Clean up resources
    if (prefetchRenderSprite)
    {
        prefetchRenderSprite->deleteSprite();
        delete prefetchRenderSprite;
        prefetchRenderSprite = nullptr;
    }

    if (prefetchMutex)
    {
        vSemaphoreDelete(prefetchMutex);
        prefetchMutex = nullptr;
    }

    if (prefetchQueue)
    {
        vQueueDelete(prefetchQueue);
        prefetchQueue = nullptr;
    }

    prefetchTaskHandle = nullptr;

}

/**
 * @brief Enqueue a tile for background prefetch
 *
 * @details Adds a tile to the prefetch queue for background loading. Non-blocking.
 *
 * @param filePath Path to the tile file
 * @param isVectorMap True if vector map, false if PNG
 */
void Maps::enqueuePrefetch(const char* filePath, bool isVectorMap)
{
    if (!prefetchQueue || !prefetchTaskRunning)
        return;

    PrefetchRequest request;
    strncpy(request.filePath, filePath, sizeof(request.filePath) - 1);
    request.filePath[sizeof(request.filePath) - 1] = '\0';
    request.isVectorMap = isVectorMap;

    // Non-blocking enqueue (don't wait if queue is full)
    xQueueSend(prefetchQueue, &request, 0);
}

/**
 * @brief Prefetch initial ring of tiles around the 3x3 grid
 *
 * @details Called after initial map load to prefetch the outer ring in all directions.
 *          This gives the prefetch system a head start before user scrolls.
 *
 * @param centerX X coordinate of center tile
 * @param centerY Y coordinate of center tile
 * @param zoom Current zoom level
 */
void Maps::prefetchInitialRing(uint32_t centerX, uint32_t centerY, uint8_t zoom)
{
    // FGB uses full viewport re-render, no tile prefetch needed
    if (!prefetchQueue || !prefetchTaskRunning || mapSet.vectorMap)
        return;

    // Prefetch outer ring (distance 2) in all directions
    for (int dx = -2; dx <= 2; dx++)
    {
        for (int dy = -2; dy <= 2; dy++)
        {
            // Skip inner 3x3 grid (already rendered)
            if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) continue;

            uint32_t tileX = centerX + dx;
            uint32_t tileY = centerY + dy;

            char filePath[255];
            snprintf(filePath, sizeof(filePath), mapRenderFolder, zoom, tileX, tileY);
            enqueuePrefetch(filePath, false);
        }
    }
}

/**
 * @brief Enqueue tiles in scroll direction for background prefetch
 *
 * @details Enqueues multiple rows/columns of tiles ahead in the scroll direction.
 *          Number of rows/columns based on cache capacity (min 3, max cache size, multiples of 3).
 *
 * @param centerX X coordinate of center tile
 * @param centerY Y coordinate of center tile
 * @param zoom Current zoom level
 * @param dirX Scroll direction X (-1 left, 0 none, 1 right)
 * @param dirY Scroll direction Y (-1 up, 0 none, 1 down)
 */
void Maps::enqueueSurroundingTiles(uint32_t centerX, uint32_t centerY, uint8_t zoom, int8_t dirX, int8_t dirY)
{
    // FGB uses full viewport re-render, no tile prefetch needed
    if (!prefetchQueue || !prefetchTaskRunning || mapSet.vectorMap)
        return;

    if (dirX == 0 && dirY == 0)
        return;

    // Prefetch only 1 row/column (3 tiles) in scroll direction at distance 2
    for (int offset = -1; offset <= 1; offset++)
    {
        uint32_t tileX, tileY;

        if (dirX != 0)
        {
            // Horizontal scroll: prefetch 1 column
            tileX = centerX + (dirX * 2);
            tileY = centerY + offset;
        }
        else
        {
            // Vertical scroll: prefetch 1 row
            tileX = centerX + offset;
            tileY = centerY + (dirY * 2);
        }

        char filePath[255];
        snprintf(filePath, sizeof(filePath), mapRenderFolder, zoom, tileX, tileY);
        enqueuePrefetch(filePath, false);
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



// ============================================================================
// FlatGeobuf Tile-based Rendering Implementation
// ============================================================================

/**
 * @brief Convert geographic coordinates to pixel coordinates
 *
 * @param lon Longitude
 * @param lat Latitude
 * @param viewport Viewport bounding box
 * @param px Output pixel X
 * @param py Output pixel Y
 */
void Maps::fgbCoordToPixel(double lon, double lat, const FgbBbox& viewport, int16_t& px, int16_t& py)
{
    // Use linear interpolation within the viewport bbox
    // Cast to float for ESP32-S3 FPU hardware acceleration (32-bit single precision)
    const float viewWidth = static_cast<float>(viewport.maxX - viewport.minX);
    const float viewHeight = static_cast<float>(viewport.maxY - viewport.minY);
    const float xNorm = static_cast<float>(lon - viewport.minX) / viewWidth;
    const float yNorm = static_cast<float>(viewport.maxY - lat) / viewHeight; // Y inverted

    // Clamp to valid pixel range to avoid overflow with out-of-bounds coordinates
    const int32_t pxRaw = static_cast<int32_t>(xNorm * static_cast<float>(tileWidth));
    const int32_t pyRaw = static_cast<int32_t>(yNorm * static_cast<float>(tileHeight));

    px = static_cast<int16_t>(std::max(-1000, std::min(2000, pxRaw)));
    py = static_cast<int16_t>(std::max(-1000, std::min(2000, pyRaw)));
}

/**
 * @brief Render a LineString feature with bbox culling
 */
void Maps::renderFgbLineString(const FgbFeature& feature, const FgbBbox& viewport, TFT_eSprite& map)
{
    if (feature.coordinates.size() < 2)
        return;

    // Convert all coordinates and compute bbox in single pass
    const size_t numCoords = feature.coordinates.size();
    MemoryGuard<int16_t> pxGuard(numCoords, 5);
    MemoryGuard<int16_t> pyGuard(numCoords, 5);
    int16_t* pxArr = pxGuard.get();
    int16_t* pyArr = pyGuard.get();

    if (!pxArr || !pyArr)
        return;

    int16_t minPx = INT16_MAX, maxPx = INT16_MIN;
    int16_t minPy = INT16_MAX, maxPy = INT16_MIN;

    for (size_t i = 0; i < numCoords; i++)
    {
        fgbCoordToPixel(feature.coordinates[i].x, feature.coordinates[i].y, viewport, pxArr[i], pyArr[i]);
        minPx = std::min(minPx, pxArr[i]);
        maxPx = std::max(maxPx, pxArr[i]);
        minPy = std::min(minPy, pyArr[i]);
        maxPy = std::max(maxPy, pyArr[i]);
    }

    // Bbox culling: skip if completely outside screen
    if (maxPx < 0 || minPx >= tileWidth || maxPy < 0 || minPy >= tileHeight)
        return;

    // Draw line segments
    uint16_t color = feature.properties.colorRgb565;
    for (size_t i = 1; i < numCoords; i++)
    {
        map.drawLine(pxArr[i-1], pyArr[i-1], pxArr[i], pyArr[i], color);
    }
}

/**
 * @brief Render a Polygon feature with bbox culling
 */
void Maps::renderFgbPolygon(const FgbFeature& feature, const FgbBbox& viewport, TFT_eSprite& map)
{
    if (feature.coordinates.size() < 3)
        return;

    // Convert coordinates to pixel arrays and compute bbox
    size_t numPoints = feature.coordinates.size();
    MemoryGuard<int> pxGuard(numPoints, 6);
    MemoryGuard<int> pyGuard(numPoints, 6);
    int* px = pxGuard.get();
    int* py = pyGuard.get();

    if (!px || !py)
        return;

    int minPx = INT_MAX, maxPx = INT_MIN;
    int minPy = INT_MAX, maxPy = INT_MIN;

    for (size_t i = 0; i < numPoints; i++)
    {
        int16_t x, y;
        fgbCoordToPixel(feature.coordinates[i].x, feature.coordinates[i].y, viewport, x, y);
        px[i] = x;
        py[i] = y;
        minPx = std::min(minPx, (int)x);
        maxPx = std::max(maxPx, (int)x);
        minPy = std::min(minPy, (int)y);
        maxPy = std::max(maxPy, (int)y);
    }

    // Bbox culling: skip if completely outside screen
    if (maxPx < 0 || minPx >= tileWidth || maxPy < 0 || minPy >= tileHeight)
        return;

    uint16_t fillColor = feature.properties.colorRgb565;
    uint16_t borderColor = darkenRGB565(feature.properties.colorRgb565, 0.3f);

    // Fill polygon if enabled
    if (fillPolygons)
    {
        fillPolygonGeneral(map, px, py, numPoints, fillColor, 0, 0);
    }

    // Draw border
    for (size_t i = 0; i < numPoints; i++)
    {
        size_t next = (i + 1) % numPoints;
        map.drawLine(px[i], py[i], px[next], py[next], borderColor);
    }
}

/**
 * @brief Render a Point feature
 */
void Maps::renderFgbPoint(const FgbFeature& feature, const FgbBbox& viewport, TFT_eSprite& map)
{
    if (feature.coordinates.empty())
        return;

    int16_t px, py;
    fgbCoordToPixel(feature.coordinates[0].x, feature.coordinates[0].y, viewport, px, py);

    if (px >= 0 && px < tileWidth && py >= 0 && py < tileHeight)
    {
        uint16_t color = feature.properties.colorRgb565;
        map.fillCircle(px, py, 3, color);
    }
}

/**
 * @brief Render a single FGB feature based on geometry type
 */
void Maps::renderFgbFeature(const FgbFeature& feature, const FgbBbox& viewport, TFT_eSprite& map)
{
    switch (feature.geometryType)
    {
        case FgbGeometryType::LineString:
        case FgbGeometryType::MultiLineString:
            renderFgbLineString(feature, viewport, map);
            break;

        case FgbGeometryType::Polygon:
        case FgbGeometryType::MultiPolygon:
            renderFgbPolygon(feature, viewport, map);
            break;

        case FgbGeometryType::Point:
        case FgbGeometryType::MultiPoint:
            renderFgbPoint(feature, viewport, map);
            break;

        default:
            break;
    }
}

/**
 * @brief Render FlatGeobuf viewport
 *
 * @details Main entry point for FGB rendering. Opens layer files if needed,
 *          queries R-Tree for visible features, sorts by priority, and renders.
 *
 * @param centerLat Center latitude
 * @param centerLon Center longitude
 * @param zoom Zoom level
 * @param map Target sprite for rendering
 * @return true if rendering succeeded
 */
bool Maps::renderFgbViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite& map)
{
    // Calculate center tile coordinates using Web Mercator projection
    // Use float math with f suffix for ESP32-S3 FPU hardware acceleration
    const float latRad = centerLat * (float)M_PI / 180.0f;
    const float n = static_cast<float>(1u << zoom);  // 2^zoom using bit shift (faster than powf)
    const int centerTileX = static_cast<int>((centerLon + 180.0f) / 360.0f * n);
    const int centerTileY = static_cast<int>((1.0f - logf(tanf(latRad) + 1.0f / cosf(latRad)) / (float)M_PI) / 2.0f * n);

    // Calculate viewport bbox based on exact 3x3 tile grid boundaries
    // This ensures features align exactly with tile boundaries
    const int minTileX = centerTileX - 1;
    const int maxTileX = centerTileX + 2;  // +2 to get right edge of tile +1
    const int minTileY = centerTileY - 1;
    const int maxTileY = centerTileY + 2;  // +2 to get bottom edge of tile +1

    const float maxLat = atanf(sinhf((float)M_PI * (1.0f - 2.0f * static_cast<float>(minTileY) / n))) * 180.0f / (float)M_PI;
    const float minLon = static_cast<float>(minTileX) / n * 360.0f - 180.0f;
    const float minLat = atanf(sinhf((float)M_PI * (1.0f - 2.0f * static_cast<float>(maxTileY) / n))) * 180.0f / (float)M_PI;
    const float maxLon = static_cast<float>(maxTileX) / n * 360.0f - 180.0f;

    FgbBbox viewport;
    viewport.minX = minLon;
    viewport.maxX = maxLon;
    viewport.minY = minLat;
    viewport.maxY = maxLat;

    // Clear map with white background
    map.fillSprite(TFT_WHITE);

    // Collect features from all tiles (3x3 grid around center)
    struct RenderItem {
        FgbFeature feature;
        uint8_t priority;
    };
    std::vector<RenderItem> renderQueue;

    int tilesLoaded = 0;
    size_t totalFeatures = 0;

    // Load 3x3 tiles around center
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int tileX = centerTileX + dx;
            int tileY = centerTileY + dy;

            // Build tile path: /sdcard/FGBMAP/{zoom}/{x}/{y}.fgb
            char tilePath[128];
            snprintf(tilePath, sizeof(tilePath), "/sdcard/FGBMAP/%d/%d/%d.fgb",
                     zoom, tileX, tileY);

            // Try to open and read tile
            FgbReader reader;
            if (!reader.open(tilePath))
            {
                // Tile doesn't exist - skip silently
                continue;
            }

            tilesLoaded++;

            // For tile-based FGB, read ALL features (file is small, no bbox query needed)
            // The R-Tree is small enough to fit in memory
            auto offsets = reader.queryBbox(viewport, 5000);

            std::vector<FgbFeature> features;
            reader.readFeaturesSequential(offsets, features, zoom);

            for (const auto& feature : features)
            {
                renderQueue.push_back({feature, feature.properties.getPriority()});
            }

            totalFeatures += features.size();
            reader.close();

            // Small delay between tiles to avoid SD saturation
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // Sort by priority (lower = render first = behind)
    std::sort(renderQueue.begin(), renderQueue.end(),
              [](const RenderItem& a, const RenderItem& b) {
                  return a.priority < b.priority;
              });

    // Render all features
    for (const auto& item : renderQueue)
    {
        renderFgbFeature(item.feature, viewport, map);
    }

    return true;
}

