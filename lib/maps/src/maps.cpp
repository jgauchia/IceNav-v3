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

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern TrackVector trackData; /**< Vector containing track waypoints */
const char* TAG = "Maps";

// Tile cache system static variables
std::vector<Maps::CachedTile> Maps::tileCache;
size_t Maps::maxCachedTiles = 0;
uint32_t Maps::cacheAccessCounter = 0;

// Background prefetch system static variables (multi-core)
QueueHandle_t Maps::prefetchQueue = nullptr;
TaskHandle_t Maps::prefetchTaskHandle = nullptr;
SemaphoreHandle_t Maps::prefetchMutex = nullptr;
volatile bool Maps::prefetchTaskRunning = false;


/**
 * @brief Map Class constructor
 *
 * @details Initializes the Maps class with default polygon filling enabled and logs completion status.
 */
Maps::Maps() : fillPolygons(true),
               navLastLat_(0), navLastLon_(0), navLastZoom_(0), navNeedsRender_(true)
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

    // 1. mapTempSprite: El canvas virtual de rendering de 3x3 tiles (768x768). Permanente. (~1.1MB)
    // Ocupa ~1.1MB de PSRAM, asignado una única vez al inicio.
    Maps::mapTempSprite.createSprite(Maps::tileWidth, Maps::tileHeight);

    // 2. mapSprite: El buffer para el canvas LVGL (también 768x768). Permanente. (~1.1MB)
    // Ocupa ~1.1MB, asignado una única vez al inicio.
    Maps::mapSprite.createSprite(Maps::tileWidth, Maps::tileHeight);
    Maps::mapBuffer = Maps::mapSprite.getBuffer(); // Enlazar este buffer al canvas LVGL

    // Pre-allocate scroll sprites (avoid allocation during scroll)
    Maps::preloadSprite.deleteSprite();
    Maps::preloadSprite.createSprite(mapTileSize * 2, mapTileSize * 2);

    Maps::oldMapTile = {};     // Old Map tile coordinates and zoom
    Maps::currentMapTile = {}; // Current Map tile coordinates and zoom
    Maps::roundMapTile = {};    // Boundaries Map tiles
    Maps::navArrowPosition = {0, 0};              // Map Arrow position

    Maps::totalBounds = {90.0f, -90.0f, 180.0f, -180.0f};

    // Pre-allocate projection buffers to avoid runtime allocation
    // 2048 points is sufficient for most complex features (roads, boundaries)
    // This works with or without PSRAM, utilizing available heap
    projBuf16X.reserve(2048);
    projBuf16Y.reserve(2048);
    projBuf32X.reserve(2048);
    projBuf32Y.reserve(2048);
    polyScanlineBuf.reserve(2048);
    
    // Initialize tile cache system
    initTileCache();

    // Initialize background prefetch system (multi-core)
    initPrefetchSystem();
}

/**
 * @brief Delete map screen and release PSRAM
 *
 * @details Deletes the main map sprite to free up PSRAM.
 */
void Maps::deleteMapScrSprites()
{

    Maps::preloadSprite.deleteSprite();

    // Clear tile cache to free memory
    clearTileCache();
}

/**
 * @brief Create map screen
 *
 * @details Creates the main map sprite with 768x768 for virtual canvas scrolling.
 *          This allows smooth panning without re-rendering.
 */
void Maps::createMapScrSprites()
{
    Maps::mapBuffer = Maps::mapSprite.getBuffer(); // Asegurar que el puntero esté correcto
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
    // Dynamic Prefetch System Control
    if (mapSet.vectorMap)
    {
        if (prefetchTaskRunning) stopPrefetchSystem();
    }
    else
    {
        if (!prefetchTaskRunning) initPrefetchSystem();
    }

    // Clear cache if zoom level changed (tiles are not compatible between zoom levels)
    if (Maps::zoomLevel != zoom && Maps::zoomLevel != 0)
    {
        ESP_LOGI(TAG, "Zoom level changed from %d to %d - clearing cache", Maps::zoomLevel, zoom);
        clearTileCache();
    }

    Maps::zoomLevel = zoom;

    const float lat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
    const float lon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

    // NAV vector map rendering path
    if (mapSet.vectorMap)
    {
        // Check if position changed enough to require re-render
        // Use ~10m threshold (approx 0.0001 degrees)
        constexpr float POS_THRESHOLD = 0.0001f;
        bool posChanged = fabsf(lat - navLastLat_) > POS_THRESHOLD ||
                          fabsf(lon - navLastLon_) > POS_THRESHOLD ||
                          zoom != navLastZoom_;

        if (posChanged || navNeedsRender_)
        {
            // Fill with background color before rendering to avoid black areas during scroll
            Maps::mapTempSprite.fillScreen(0xE6D2);  // Light beige/tan map background
            Maps::isMapFound = renderNavViewport(lat, lon, zoom, Maps::mapTempSprite);
            if (!Maps::isMapFound)
            {
                ESP_LOGW(TAG, "NAV: No map data found");
                Maps::mapTempSprite.fillScreen(TFT_BLACK);
                Maps::showNoMap(Maps::mapTempSprite);
            }
            // Update cache
            navLastLat_ = lat;
            navLastLon_ = lon;
            navLastZoom_ = zoom;
            navNeedsRender_ = false;
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
    mapHeading = mapSet.mapRotationComp ? globalSensorData.heading : gps.gpsData.heading;
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
        // Manual mode: copy without rotation, viewport controlled by LVGL lv_obj_set_pos
        Maps::mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
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

    // Calculate max offsets based on screen size (leave 10px safety margin)
    const int16_t maxOffsetX = (tileWidth - mapScrWidth) / 2 - 10;
    const int16_t maxOffsetY = (tileHeight - mapScrHeight) / 2 - 10;

    // Clamp offsets to prevent canvas edge from showing
    if (Maps::offsetX > maxOffsetX) Maps::offsetX = maxOffsetX;
    if (Maps::offsetX < -maxOffsetX) Maps::offsetX = -maxOffsetX;
    if (Maps::offsetY > maxOffsetY) Maps::offsetY = maxOffsetY;
    if (Maps::offsetY < -maxOffsetY) Maps::offsetY = -maxOffsetY;

    Maps::scrollUpdated = false;
    Maps::followGps = false;

    // Dynamic threshold based on available margin (can't exceed maxOffset)
    const int16_t thresholdX = std::min((int16_t)Maps::scrollThreshold, maxOffsetX);
    const int16_t thresholdY = std::min((int16_t)Maps::scrollThreshold, maxOffsetY);
    const int16_t threshold = std::min(thresholdX, thresholdY);
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
        Maps::redrawMap = true; // Force redraw to sync image with new offsets
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
    int miny = py[0], maxy = py[0];
    for (int i = 1; i < numPoints; ++i) 
    {
        if (py[i] < miny) 
            miny = py[i];
        if (py[i] > maxy)
            maxy = py[i];
    }

    int *xints = nullptr;
    
    // Use persistent buffer for coordinate arrays
    if (polyScanlineBuf.capacity() < numPoints) polyScanlineBuf.reserve(numPoints);
    // No resize needed as we use it as a raw buffer and track count via 'nodes'
    xints = polyScanlineBuf.data();

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
        // Only run when in PNG Map mode and showing the Map tile on main screen
        if (mapSet.vectorMap || lv_screen_active() != mainScreen || activeTile != MAP)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Wait for a request with timeout
        if (xQueueReceive(prefetchQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (prefetchMutex && xSemaphoreTake(prefetchMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                // Check if already cached using hash
                uint32_t tileHash = mapsInstance->calculateTileHash(request.filePath);
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
                    // FS Warming: pre-read file to trigger SD/VFS cache
                    FILE* pngFile = fopen(request.filePath, "rb");
                    if (pngFile)
                    {
                        static char warmingBuffer[1024]; // Small static buffer to save stack
                        while (fread(warmingBuffer, 1, sizeof(warmingBuffer), pngFile) > 0) {}
                        fclose(pngFile);
                    }
                }
                xSemaphoreGive(prefetchMutex);
            }
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
    if (prefetchTaskRunning || mapSet.vectorMap)
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

    // Start prefetch task on Core 0 (same as GPS, low priority)
    prefetchTaskRunning = true;
    BaseType_t result = xTaskCreatePinnedToCore(
        prefetchTask,           // Task function
        "PrefetchTask",         // Task name
        3072,                   // Stack size (3KB)
        this,                   // Pass Maps instance for renderTile access
        1,                      // Priority
        &prefetchTaskHandle,    // Task handle
        0                       // Core 0
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create prefetch task");
        prefetchTaskRunning = false;
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
 */
void Maps::enqueuePrefetch(const char* filePath)
{
    if (!prefetchQueue || !prefetchTaskRunning)
        return;

    PrefetchRequest request;
    strncpy(request.filePath, filePath, sizeof(request.filePath) - 1);
    request.filePath[sizeof(request.filePath) - 1] = '\0';

    // Non-blocking enqueue (don't wait if queue is full)
    xQueueSend(prefetchQueue, &request, 0);
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
        enqueuePrefetch(filePath);
    }
}
        
// ============================================================================
// NAV Tile-based Rendering Implementation
// ============================================================================

/**
 * @brief Convert int32 scaled coordinates to pixel coordinates
 *
 * @param lon Longitude (scaled by COORD_SCALE)
 * @param lat Latitude (scaled by COORD_SCALE)
 * @param viewport Viewport bounding box (scaled coords)
 * @param px Output pixel X
 * @param py Output pixel Y
 */
void Maps::navCoordToPixel(int32_t lon, int32_t lat, const NavBbox& viewport, int16_t& px, int16_t& py)
{
    // Use integer math then convert to float for final calculation
    const float viewWidth = static_cast<float>(viewport.maxLon - viewport.minLon);
    const float viewHeight = static_cast<float>(viewport.maxLat - viewport.minLat);
    const float xNorm = static_cast<float>(lon - viewport.minLon) / viewWidth;
    const float yNorm = static_cast<float>(viewport.maxLat - lat) / viewHeight; // Y inverted

    const int32_t pxRaw = static_cast<int32_t>(xNorm * static_cast<float>(tileWidth));
    const int32_t pyRaw = static_cast<int32_t>(yNorm * static_cast<float>(tileHeight));

    px = static_cast<int16_t>(std::max(-1000, std::min(2000, pxRaw)));
    py = static_cast<int16_t>(std::max(-1000, std::min(2000, pyRaw)));
}

/**
 * @brief Render a LineString feature with bbox culling.
 * 
 * @param feature  The vector line feature to render.
 * @param viewport Current view bounding box for projection.
 * @param map      Target sprite to draw on.
 */
void Maps::renderNavLineString(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map)
{
    if (feature.coords.size() < 2)
        return;

    const size_t numCoords = feature.coords.size();
    
    // Zero-Allocation Projection Pipeline
    if (projBuf16X.capacity() < numCoords) projBuf16X.reserve(numCoords * 1.5);
    if (projBuf16Y.capacity() < numCoords) projBuf16Y.reserve(numCoords * 1.5);
    
    // Resize without reallocating if capacity is sufficient
    projBuf16X.resize(numCoords);
    projBuf16Y.resize(numCoords);

    int16_t* pxArr = projBuf16X.data();
    int16_t* pyArr = projBuf16Y.data();

    int16_t minPx = INT16_MAX, maxPx = INT16_MIN;
    int16_t minPy = INT16_MAX, maxPy = INT16_MIN;

    for (size_t i = 0; i < numCoords; i++)
    {
        navCoordToPixel(feature.coords[i].lon, feature.coords[i].lat, viewport, pxArr[i], pyArr[i]);
        minPx = std::min(minPx, pxArr[i]);
        maxPx = std::max(maxPx, pxArr[i]);
        minPy = std::min(minPy, pyArr[i]);
        maxPy = std::max(maxPy, pyArr[i]);
    }

    // Bbox culling: skip if completely outside screen
    if (maxPx < 0 || minPx >= tileWidth || maxPy < 0 || minPy >= tileHeight)
        return;

    uint16_t color = feature.properties.colorRgb565;
    uint8_t width = feature.properties.getWidth();

    // For thick lines, add round join at first vertex
    if (width > 2)
    {
        map.fillCircle(pxArr[0], pyArr[0], width / 2, color);
    }

    for (size_t i = 1; i < numCoords; i++)
    {
        if (width <= 2)
        {
            // Fast path for thin lines
            map.drawLine(pxArr[i-1], pyArr[i-1], pxArr[i], pyArr[i], color);
            if (width == 2)
            {
                // Draw parallel line for width 2
                map.drawLine(pxArr[i-1]+1, pyArr[i-1], pxArr[i]+1, pyArr[i], color);
            }
        }
        else
        {
            // Use drawWideLine for thicker lines
            map.drawWideLine(pxArr[i-1], pyArr[i-1], pxArr[i], pyArr[i], width, color);
            // Round join at vertex to smooth curves
            map.fillCircle(pxArr[i], pyArr[i], width / 2, color);
        }
    }
}

/**
 * @brief Render a Polygon feature with bbox culling.
 * 
 * @param feature  The vector polygon feature to render.
 * @param viewport Current view bounding box for projection.
 * @param map      Target sprite to draw on.
 */
void Maps::renderNavPolygon(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map)
{
    if (feature.coords.size() < 3)
        return;

    size_t numPoints = feature.coords.size();
    
    // Zero-Allocation Projection Pipeline
    if (projBuf32X.capacity() < numPoints) projBuf32X.reserve(numPoints * 1.5);
    if (projBuf32Y.capacity() < numPoints) projBuf32Y.reserve(numPoints * 1.5);

    // Resize without reallocating if capacity is sufficient
    projBuf32X.resize(numPoints);
    projBuf32Y.resize(numPoints);

    int* px = projBuf32X.data();
    int* py = projBuf32Y.data();

    int minPx = INT_MAX, maxPx = INT_MIN;
    int minPy = INT_MAX, maxPy = INT_MIN;

    for (size_t i = 0; i < numPoints; i++)
    {
        int16_t x, y;
        navCoordToPixel(feature.coords[i].lon, feature.coords[i].lat, viewport, x, y);
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

    // Skip polygons with oversized bbox (likely crossing tile boundary incorrectly)
    int bboxWidth = maxPx - minPx;
    int bboxHeight = maxPy - minPy;
    if (bboxWidth > tileWidth * 2 || bboxHeight > tileHeight * 2)
        return;

    uint16_t fillColor = feature.properties.colorRgb565;
    uint16_t borderColor = darkenRGB565(feature.properties.colorRgb565, 0.3f);

    if (fillPolygons)
    {
        fillPolygonGeneral(map, px, py, numPoints, fillColor, 0, 0);
    }

    for (size_t i = 0; i < numPoints; i++)
    {
        size_t next = (i + 1) % numPoints;
        map.drawLine(px[i], py[i], px[next], py[next], borderColor);
    }
}

/**
 * @brief Render a Point feature.
 * 
 * @param feature  The vector point feature to render.
 * @param viewport Current view bounding box for projection.
 * @param map      Target sprite to draw on.
 */
void Maps::renderNavPoint(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map)
{
    if (feature.coords.empty())
        return;

    int16_t px, py;
    navCoordToPixel(feature.coords[0].lon, feature.coords[0].lat, viewport, px, py);

    if (px >= 0 && px < tileWidth && py >= 0 && py < tileHeight)
    {
        uint16_t color = feature.properties.colorRgb565;
        map.fillCircle(px, py, 3, color);
    }
}

/**
 * @brief Render a single NAV feature based on geometry type.
 * 
 * @param feature  The feature to render.
 * @param viewport Current view bounding box for projection.
 * @param map      Target sprite to draw on.
 */
void Maps::renderNavFeature(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map)
{
    switch (feature.geomType)
    {
        case NavGeomType::LineString:
            renderNavLineString(feature, viewport, map);
            break;

        case NavGeomType::Polygon:
            renderNavPolygon(feature, viewport, map);
            break;

        case NavGeomType::Point:
            renderNavPoint(feature, viewport, map);
            break;

        default:
            break;
    }
}

/**
 * @brief Render NAV viewport
 *
 * @details Main entry point for NAV tile rendering. Loads 3x3 tile grid,
 *          sorts features by priority, and renders to sprite.
 *
 * @param centerLat Center latitude
 * @param centerLon Center longitude
 * @param zoom Zoom level
 * @param map Target sprite for rendering
 * @return true if rendering succeeded
 */
bool Maps::renderNavViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite& map)
{
    // Calculate center tile coordinates using Web Mercator projection
    const float latRad = centerLat * (float)M_PI / 180.0f;
    const float n = static_cast<float>(1u << zoom);
    const int centerTileX = static_cast<int>((centerLon + 180.0f) / 360.0f * n);
    const int centerTileY = static_cast<int>((1.0f - logf(tanf(latRad) + 1.0f / cosf(latRad)) / (float)M_PI) / 2.0f * n);

    // Calculate viewport bbox (in scaled int32 coordinates)
    const int minTileX = centerTileX - 1;
    const int maxTileX = centerTileX + 2;
    const int minTileY = centerTileY - 1;
    const int maxTileY = centerTileY + 2;

    const float maxLatF = atanf(sinhf((float)M_PI * (1.0f - 2.0f * static_cast<float>(minTileY) / n))) * 180.0f / (float)M_PI;
    const float minLonF = static_cast<float>(minTileX) / n * 360.0f - 180.0f;
    const float minLatF = atanf(sinhf((float)M_PI * (1.0f - 2.0f * static_cast<float>(maxTileY) / n))) * 180.0f / (float)M_PI;
    const float maxLonF = static_cast<float>(maxTileX) / n * 360.0f - 180.0f;

    NavBbox viewport;
    viewport.minLon = static_cast<int32_t>(minLonF * COORD_SCALE);
    viewport.maxLon = static_cast<int32_t>(maxLonF * COORD_SCALE);
    viewport.minLat = static_cast<int32_t>(minLatF * COORD_SCALE);
    viewport.maxLat = static_cast<int32_t>(maxLatF * COORD_SCALE);

    // Clear map with white background
    map.fillSprite(TFT_WHITE);

    // Collect features from all tiles
    struct RenderItem {
        NavFeature feature;
        uint8_t priority;
    };
    std::vector<RenderItem> renderQueue;

    // Load 3x3 tiles around center
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int tileX = centerTileX + dx;
            int tileY = centerTileY + dy;

            char tilePath[128];
            snprintf(tilePath, sizeof(tilePath), mapVectorFolder,
                     zoom, tileX, tileY);

            NavReader reader;
            if (!reader.open(tilePath))
                continue;

            std::vector<NavFeature> features;
            reader.readAllFeatures(features, zoom);

            for (const auto& feature : features)
            {
                renderQueue.push_back({feature, feature.properties.getPriority()});
            }

            reader.close();
            vTaskDelay(pdMS_TO_TICKS(10));
            taskYIELD();
        }
    }

    // Sort by priority (needed when combining multiple tiles)
    std::sort(renderQueue.begin(), renderQueue.end(),
              [](const RenderItem& a, const RenderItem& b) {
                  return a.priority < b.priority;
              });

    // Render all features
    for (const auto& item : renderQueue)
    {
        // Bbox Pre-Culling
        if (item.feature.coords.empty()) continue;

        int32_t fMinLon = INT32_MAX, fMaxLon = INT32_MIN;
        int32_t fMinLat = INT32_MAX, fMaxLat = INT32_MIN;

        for (const auto& c : item.feature.coords)
        {
            if (c.lon < fMinLon) fMinLon = c.lon;
            if (c.lon > fMaxLon) fMaxLon = c.lon;
            if (c.lat < fMinLat) fMinLat = c.lat;
            if (c.lat > fMaxLat) fMaxLat = c.lat;
        }

        if (fMinLon > viewport.maxLon || fMaxLon < viewport.minLon || 
            fMinLat > viewport.maxLat || fMaxLat < viewport.minLat)
        {
            continue;
        }

        renderNavFeature(item.feature, viewport, map);
    }

    return true;
}

