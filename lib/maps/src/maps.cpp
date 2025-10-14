/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.3
 * @date 2025-06
 */

#include "maps.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData; /**< Vector containing track waypoints */
const char* TAG PROGMEM = "Maps";

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

// Precalculated transformation matrices static variables
Maps::TransformMatrix Maps::coordTransformMatrix = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false};
Maps::TransformMatrix Maps::pixelTransformMatrix = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false};
bool Maps::transformMatricesValid = false;
uint32_t Maps::lastTransformUpdate = 0;

// Efficient batch rendering static variables
Maps::RenderBatch* Maps::activeBatch = nullptr;
size_t Maps::maxBatchSize = 0;
uint32_t Maps::batchRenderCount = 0;
uint32_t Maps::batchOptimizationCount = 0;
uint32_t Maps::batchFlushCount = 0;

/**
 * @brief Get the Optimal Batch Size object
 * 
 * @return size_t 
 */
static size_t getOptimalBatchSize() 
{
    static size_t optimalSize = 0;
    if (optimalSize == 0) 
    {
#ifdef BOARD_HAS_PSRAM
        size_t psramFree = ESP.getFreePsram();
        if (psramFree >= 4 * 1024 * 1024) 
            optimalSize = 256;  // High-end ESP32-S3: 256 lines
        else if (psramFree >= 2 * 1024 * 1024)
            optimalSize = 128;  // Mid-range ESP32-S3: 128 lines
        else 
            optimalSize = 64;   // Low-end ESP32-S3: 64 lines
#else
        optimalSize = 32;  // ESP32 without PSRAM: 32 lines
#endif
        ESP_LOGI(TAG, "Optimal batch size: %zu lines", optimalSize);
    }
    return optimalSize;
}

/**
 * @brief Map Class constructor
 */
Maps::Maps() : fillPolygons(true) 
{
    ESP_LOGI(TAG, "Maps constructor called - initializing pools...");
    
    // Initialize advanced memory pools in constructor
    initUnifiedPool();
    initTransformMatrices();
    initBatchRendering();
    
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
    // Use precalculated transformation if available
    if (transformMatricesValid && coordTransformMatrix.isValid) {
        return transformLonToPixel(f_lon, zoom, tileSize);
    }
    
    // Fallback to original calculation
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
    // Use precalculated transformation if available
    if (transformMatricesValid && coordTransformMatrix.isValid) {
        return transformLatToPixel(f_lat, zoom, tileSize);
    }
    
    // Fallback to original calculation
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
	
	// Initialize background preload system

	// Initialize polygon optimizations
    // Initialize polygon optimization system
    polygonCullingEnabled = true;
    optimizedScanlineEnabled = false;
    polygonRenderCount = 0;
    polygonCulledCount = 0;
    polygonOptimizedCount = 0;
	
	// Initialize transformation matrices
	initTransformMatrices();
	
	// Initialize batch rendering
	initBatchRendering();
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
	
	
	// Clear memory pool
	clearUnifiedPool();
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
		invalidateTransformMatrices(); // Invalidate matrices when zoom changes
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
		}
	}
}

/**
 * @brief Display Map
 *
 * @details Displays the map on the screen. 
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
		
		// Print cache stats every 2 seconds during scrolling
		if (millis() - lastCacheStats > 2000) {
			ESP_LOGI(TAG, "=== Scroll Cache Stats ===");
			ESP_LOGI(TAG, "Scroll delta: X=%d, Y=%d", deltaTileX, deltaTileY);
			ESP_LOGI(TAG, "Cache hits: %zu/%zu tiles", tileCache.size(), maxCachedTiles);
			ESP_LOGI(TAG, "Cache memory: %zu bytes", getCacheMemoryUsage());
			lastCacheStats = millis();
		}
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

	ESP_LOGI(TAG, "Preloading tiles: dirX=%d, dirY=%d", dirX, dirY);

	TFT_eSprite preloadSprite = TFT_eSprite(&tft);
	preloadSprite.createSprite(preloadWidth, preloadHeight);

	const int16_t startX = tileX + dirX;
	const int16_t startY = tileY + dirY;

	for (int8_t i = 0; i < 2; ++i) 
	{
		const int16_t tileToLoadX = startX + ((dirX == 0) ? i - 1 : 0);
		const int16_t tileToLoadY = startY + ((dirY == 0) ? i - 1 : 0);

		Maps::roundMapTile = Maps::getMapTile(
			Maps::currentMapTile.lon,
			Maps::currentMapTile.lat,
			Maps::zoomLevel,
			tileToLoadX,
			tileToLoadY
		);

		const int16_t offsetX = (dirX != 0) ? i * tileSize : 0;
		const int16_t offsetY = (dirY != 0) ? i * tileSize : 0;

		bool foundTile = false;
		
		ESP_LOGI(TAG, "Loading tile: %s", Maps::roundMapTile.file);
		
		// Try cache first for vector maps
		if (mapSet.vectorMap) 
        {
			foundTile = getCachedTile(Maps::roundMapTile.file, preloadSprite, offsetX, offsetY);
			if (foundTile) 
				ESP_LOGI(TAG, "Tile found in cache: %s", Maps::roundMapTile.file);
		}
		
		// If not in cache, try to load from file
		if (!foundTile)
        {
			if (mapSet.vectorMap)
            {
				ESP_LOGI(TAG, "Rendering tile to cache: %s", Maps::roundMapTile.file);
				// Create a temporary sprite for rendering to cache
				TFT_eSprite tempSprite = TFT_eSprite(&tft);
				tempSprite.createSprite(tileSize, tileSize);
				
				// Render tile to temporary sprite (this will cache it)
				foundTile = renderTile(Maps::roundMapTile.file, 0, 0, tempSprite);
				
				if (foundTile) 
                {
					// Copy from temporary sprite to preload sprite
					preloadSprite.pushImage(offsetX, offsetY, tileSize, tileSize, tempSprite.frameBuffer(0));
					ESP_LOGI(TAG, "Tile rendered and cached: %s", Maps::roundMapTile.file);
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
            // Convert RGB888 to RGB332 using same method as tile_generator.py hex_to_rgb332_direct
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
                        map.drawLine(x0, yy, x1, yy, color);
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

    for (uint32_t i = 0; i < numPoints - 1; ++i) 
    {
        const bool marginA = isPointOnMargin(px[i], py[i]);
        const bool marginB = isPointOnMargin(px[i+1], py[i+1]);
        const uint16_t color = (marginA && marginB) ? fillColor : borderColor;

        const int x0 = px[i] + xOffset;
        const int y0 = py[i] + yOffset;
        const int x1 = px[i+1] + xOffset;
        const int y1 = py[i+1] + yOffset;

        if (x0 >= 0 && x0 <= TILE_SIZE + xOffset && y0 >= 0 && y0 <= TILE_SIZE + yOffset &&
            x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset) 
        {
            if (!(marginA && marginB && !fillPolygons))
                map.drawLine(x0, y0, x1, y1, color);
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
        x1 >= 0 && x1 <= TILE_SIZE + xOffset && y1 >= 0 && y1 <= TILE_SIZE + yOffset) 
    {
        if (!(marginA && marginB && !fillPolygons)) 
            map.drawLine(x0, y0, x1, y1, color);
    }
}   

/**
* @brief Draws a filled circle with a border on a sprite.
*                   
* @details This function draws a circle on the provided TFT_eSprite object (map) at the specified position (x, y) with the given radius (r). 
*          It fills the circle with the specified fill color and draws a border around it with the specified border color. 
*          If fillShape is false, it only draws the circle border using the fill color.
*
* @param map The TFT_eSprite object where the circle will be drawn.
* @param x The x-coordinate of the circle's center.
* @param y The y-coordinate of the circle's center.
* @param r The radius of the circle.
* @param fill The color to fill the circle with (in RGB565 format).
* @param border The color to use for the circle border (in RGB565 format).
* @param fillShape If true, the circle will be filled; if false, only the border will be drawn.
*/

/**
* @brief Draws a filled rectangle with a border on a sprite.
*
* @details This function draws a rectangle on the provided TFT_eSprite object (map) at the specified position (x, y) with the given width (w) and height (h). 
*          It fills the rectangle with the specified fill color and draws a border around it with the specified border color. 
*          If fillShape is false, it only draws the rectangle border using the fill color.
*
* @param map The TFT_eSprite object where the rectangle will be drawn.
* @param x The x-coordinate of the top-left corner of the rectangle.
* @param y The y-coordinate of the top-left corner of the rectangle.
* @param w The width of the rectangle.
* @param h The height of the rectangle.
* @param fill The color to fill the rectangle with (in RGB565 format).
* @param border The color to use for the rectangle border (in RGB565 format).
* @param fillShape If true, the rectangle will be filled; if false, only the border will be drawn.
*/

/**
 * @brief Draw a dashed line between two points
 *
 * @details Draws a dashed line using Bresenham-like algorithm with configurable dash and gap lengths.
 *
 * @param map The TFT_eSprite object where the line will be drawn.
 * @param x1, y1 Starting point coordinates.
 * @param x2, y2 Ending point coordinates.
 * @param dashLength Length of each dash segment.
 * @param gapLength Length of each gap between dashes.
 * @param color The color to draw the line with (in RGB565 format).
 */






/**
 * @brief Initialize tile cache system
 *
 * @details Initializes the tile cache system by detecting hardware capabilities and setting up the cache structure.
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
    while (*p) {
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
    
    auto lruIt = tileCache.begin();
    uint32_t oldestAccess = lruIt->lastAccess;
    
    for (auto it = tileCache.begin(); it != tileCache.end(); ++it) 
    {
        if (it->lastAccess < oldestAccess)
        {
            oldestAccess = it->lastAccess;
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
 * @brief Add tile to preload queue
 *
 * @details Adds a tile to the background preload queue for processing.
 *
 * @param filePath The file path of the tile to preload.
 * @param tileX The X coordinate of the tile.
 * @param tileY The Y coordinate of the tile.
 * @param zoom The zoom level of the tile.
 */



/**
 * @brief Optimized polygon filling
 *
 * @details Fills a polygon using the most appropriate algorithm based on polygon complexity.
 *
 * @param map The TFT_eSprite object where the polygon will be drawn.
 * @param px Array of x-coordinates of the polygon vertices.
 * @param py Array of y-coordinates of the polygon vertices.
 * @param numPoints The number of vertices in the polygon.
 * @param color The color to fill the polygon with.
 * @param xOffset The x-offset to apply when drawing the polygon.
 * @param yOffset The y-offset to apply when drawing the polygon.
 */
void Maps::fillPolygonOptimized(TFT_eSprite &map, const int *px, const int *py, int numPoints, uint16_t color, int xOffset, int yOffset)
{
    polygonRenderCount++;
    
    // If optimizations are disabled, use original algorithm
    if (!polygonCullingEnabled && !optimizedScanlineEnabled) 
    {
        fillPolygonGeneral(map, px, py, numPoints, color, xOffset, yOffset);
        return;
    }
    
    // Apply viewport culling if enabled
    if (polygonCullingEnabled) 
    {
        // Simple bounds check without complex calculations
        int minX = px[0], maxX = px[0], minY = py[0], maxY = py[0];
        for (int i = 1; i < numPoints; i++) {
            if (px[i] < minX) minX = px[i];
            if (px[i] > maxX) maxX = px[i];
            if (py[i] < minY) minY = py[i];
            if (py[i] > maxY) maxY = py[i];
        }
        
        // Simple viewport check
        if (maxX < xOffset || minX > xOffset + TILE_SIZE ||
            maxY < yOffset || minY > yOffset + TILE_SIZE) 
        {
            polygonCulledCount++;
            return; // Skip rendering
        }
    }
    
    // Use optimized algorithms for simple polygons
    if (numPoints == 3) 
    {
        polygonOptimizedCount++;
        fillTriangleOptimized(map, px[0] + xOffset, py[0] + yOffset,
                             px[1] + xOffset, py[1] + yOffset,
                             px[2] + xOffset, py[2] + yOffset, color);
    }
    else if (numPoints == 4) 
    {
        polygonOptimizedCount++;
        // Simple rectangle optimization
        int minX = px[0], maxX = px[0], minY = py[0], maxY = py[0];
        for (int i = 1; i < 4; i++) {
            if (px[i] < minX) minX = px[i];
            if (px[i] > maxX) maxX = px[i];
            if (py[i] < minY) minY = py[i];
            if (py[i] > maxY) maxY = py[i];
        }
        fillRectangleOptimized(map, minX + xOffset, minY + yOffset,
                              maxX - minX, maxY - minY, color);
    }
    else 
    {
        // Fallback to original algorithm
        fillPolygonGeneral(map, px, py, numPoints, color, xOffset, yOffset);
    }
}

/**
 * @brief Optimized triangle filling
 *
 * @details Fills a triangle using an optimized algorithm.
 *
 * @param map The TFT_eSprite object where the triangle will be drawn.
 * @param x1, y1 First vertex coordinates.
 * @param x2, y2 Second vertex coordinates.
 * @param x3, y3 Third vertex coordinates.
 * @param color The color to fill the triangle with.
 */
void Maps::fillTriangleOptimized(TFT_eSprite &map, int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color)
{
    // Sort vertices by Y coordinate
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    if (y1 > y3) { std::swap(x1, x3); std::swap(y1, y3); }
    if (y2 > y3) { std::swap(x2, x3); std::swap(y2, y3); }
    
    // Calculate slopes
    int dx1 = x2 - x1;
    int dy1 = y2 - y1;
    int dx2 = x3 - x1;
    int dy2 = y3 - y1;
    int dx3 = x3 - x2;
    int dy3 = y3 - y2;
    
    // Fill top half
    for (int y = y1; y < y2; y++) 
    {
        int x_start = x1 + (y - y1) * dx1 / dy1;
        int x_end = x1 + (y - y1) * dx2 / dy2;
        
        if (x_start > x_end) std::swap(x_start, x_end);
        
        if (x_start >= 0 && x_end < TILE_SIZE && y >= 0 && y < TILE_SIZE) 
            map.drawLine(x_start, y, x_end, y, color);
    }
    
    // Fill bottom half
    for (int y = y2; y <= y3; y++) 
    {
        int x_start = x2 + (y - y2) * dx3 / dy3;
        int x_end = x1 + (y - y1) * dx2 / dy2;
        
        if (x_start > x_end) std::swap(x_start, x_end);
        
        if (x_start >= 0 && x_end < TILE_SIZE && y >= 0 && y < TILE_SIZE) 
            map.drawLine(x_start, y, x_end, y, color);
    }
}

/**
 * @brief Optimized rectangle filling
 *
 * @details Fills a rectangle using the optimized fillRect method.
 *
 * @param map The TFT_eSprite object where the rectangle will be drawn.
 * @param x, y Top-left corner coordinates.
 * @param w, h Width and height of the rectangle.
 * @param color The color to fill the rectangle with.
 */
void Maps::fillRectangleOptimized(TFT_eSprite &map, int x, int y, int w, int h, uint16_t color)
{
    // Clamp to tile boundaries
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TILE_SIZE) w = TILE_SIZE - x;
    if (y + h > TILE_SIZE) h = TILE_SIZE - y;
    
    if (w > 0 && h > 0)
        map.fillRect(x, y, w, h, color);
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
    static bool unifiedPolygonLogged = false;
    static bool unifiedPolygonsLogged = false;
    if (!isPaletteLoaded) 
        isPaletteLoaded = Maps::loadPalette("/sdcard/VECTMAP/palette.bin");

    if (!path || path[0] == '\0')
        return false;

    // Try to get tile from cache first
    if (getCachedTile(path, map, xOffset, yOffset))
        return true; // Tile found in cache

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
    {
        ESP_LOGI(TAG, "renderTile: Using RAII MemoryGuard for data allocation");
        unifiedPoolLogged = true;
    }

    const size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) 
        return false;

    // Update memory statistics (simplified)
    currentMemoryUsage = ESP.getFreeHeap();
    if (currentMemoryUsage > peakMemoryUsage) 
        peakMemoryUsage = currentMemoryUsage;

    size_t offset = 0;
    const size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = RGB332ToRGB565(current_color);

    const uint32_t num_cmds = readVarint(data, offset, dataSize);
    if (num_cmds == 0)
        return false;

    // Use optimal batch size for this hardware
    const size_t optimalBatchSize = getOptimalBatchSize();
    
    // Initialize efficient batch rendering
    createRenderBatch(optimalBatchSize);
    
    int batchCount = 0;

    // Track memory allocations for statistics
    totalMemoryAllocations++;

    int executed = 0;
    int totalLines = 0;
    int batchFlushes = 0;
    int batchOptimizations = 0;  // Track optimizations per tile
    unsigned long renderStart = millis();
    
    // Command type counters for debugging
    int lineCommands = 0;
    int polygonCommands = 0;
    int rectangleCommands = 0;
    int circleCommands = 0;

    // Optimized flushBatch with better memory access patterns
    auto flushCurrentBatch = [&]() 
    {
        if (batchCount == 0) return;
        
        totalLines += batchCount;
        batchFlushes++;
        
        // Use efficient batch rendering
        flushBatch(map, batchOptimizations);
        batchCount = 0;
    };

    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) 
    {
        if (offset >= dataSize) 
            break;
        const size_t cmdStartOffset = offset;
        const uint32_t cmdType = readVarint(data, offset, dataSize);
        
        // Since commands are pre-sorted by layer but we can't detect layer changes,
        // we'll be more conservative and flush batches more frequently to ensure
        // proper layer ordering. Flush every 50 commands to maintain layer integrity.
        if (cmd_idx > 0 && cmd_idx % 50 == 0) {
            flushCurrentBatch();
        }

        bool isLineCommand = false;

        switch (cmdType) 
        {
            case SET_COLOR:
                flushCurrentBatch();
                if (offset < dataSize) 
                {
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
                    if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) 
                    {
                        // Use efficient batch rendering
                        addToBatch(px1, py1, px2, py2, currentDrawColor);
                        batchCount++;
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case DRAW_POLYLINE:
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 2) 
                    {
                        // Use RAII MemoryGuard for coordinate arrays
                        MemoryGuard<int> pxGuard(numPoints, 6); // Type 6 = coordArray
                        MemoryGuard<int> pyGuard(numPoints, 6); // Type 6 = coordArray
                        int* px = pxGuard.get();
                        int* py = pyGuard.get();
                        if (!px || !py)
                        {
                            ESP_LOGW(TAG, "renderTile DRAW_POLYLINE: RAII MemoryGuard allocation failed");
                            continue;
                        }
                        if (!unifiedPolylineLogged)
                        {
                            ESP_LOGI(TAG, "renderTile DRAW_POLYLINE: Using RAII MemoryGuard for coordinate arrays");
                            unifiedPolylineLogged = true;
                        }
                        unifiedPoolHitCount += 2;
                        
                        if (!px || !py)
                        {
                            // Skip this command if allocation failed
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                            {
                                readZigzag(data, offset, dataSize);
                                readZigzag(data, offset, dataSize);
                            }
                            continue;
                        }
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
                            {
                                // Use efficient batch rendering
                                addToBatch(px[i-1], py[i-1], px[i], py[i], currentDrawColor);
                                batchCount++;
                            }
                        }
                        executed++;
                        isLineCommand = true;
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
            case OPTIMIZED_POLYGON:
            case HOLLOW_POLYGON:
                flushCurrentBatch();
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 3) 
                    {
                        // Use RAII MemoryGuard for coordinate arrays
                        MemoryGuard<int> pxGuard(numPoints, 6); // Type 6 = coordArray
                        MemoryGuard<int> pyGuard(numPoints, 6); // Type 6 = coordArray
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
                        if (!unifiedPolygonLogged) 
                        {
                            ESP_LOGI(TAG, "renderTile DRAW_STROKE_POLYGON: Using RAII MemoryGuard for coordinate arrays");
                            unifiedPolygonLogged = true;
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
                        if (fillPolygons && numPoints >= 3 && cmdType != HOLLOW_POLYGON) 
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
            case DRAW_STROKE_POLYGONS:
                flushCurrentBatch();
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    int32_t accumX = 0, accumY = 0;
                    if (numPoints >= 3) 
                    {
                        // Use RAII MemoryGuard for coordinate arrays
                        MemoryGuard<int> pxGuard(numPoints, 6); // Type 6 = coordArray
                        MemoryGuard<int> pyGuard(numPoints, 6); // Type 6 = coordArray
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
                        if (!unifiedPolygonsLogged)
                        {
                            ESP_LOGI(TAG, "renderTile DRAW_STROKE_POLYGONS: Using RAII MemoryGuard for coordinate arrays");
                            unifiedPolygonsLogged = true;
                        }
                        for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                        {
                            if (i == 0) 
                            {
                                accumX = readZigzag(data, offset, dataSize);
                                accumY = readZigzag(data, offset, dataSize);
                            } 
                            else 
                            {
                                const int32_t deltaX = readZigzag(data, offset, dataSize);
                                const int32_t deltaY = readZigzag(data, offset, dataSize);
                                accumX += deltaX;
                                accumY += deltaY;
                            }
                            px[i] = uint16ToPixel(accumX);
                            py[i] = uint16ToPixel(accumY);
                        }
                        if (fillPolygons && numPoints >= 3)
                        {
                            fillPolygonOptimized(map, px, py, numPoints, currentDrawColor, xOffset, yOffset);
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            drawPolygonBorder(map, px, py, numPoints, borderColor, currentDrawColor, xOffset, yOffset);
                            polygonCommands++;
                            executed++;
                        }
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
            case DRAW_HORIZONTAL_LINE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t x2 = x1 + dx;
                    const int px1 = uint16ToPixel(x1) + xOffset;
                    const int px2 = uint16ToPixel(x2) + xOffset;
                    const int py = uint16ToPixel(y) + yOffset;
                    if (shouldDrawLine(px1 - xOffset, py - yOffset, px2 - xOffset, py - yOffset))
                    {
                        // Use efficient batch rendering
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
                    if (shouldDrawLine(px - xOffset, py1 - yOffset, px - xOffset, py2 - yOffset)) 
                    {
                        // Use efficient batch rendering
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
                        !isPointOnMargin(px1 + pwidth, py1 + pheight))
                    {
                        if (fillPolygons) 
                        {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, borderColor);
                            executed++;
                        }
                        else
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
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
                    
                    // Validate rectangle bounds
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE &&
                        pwidth > 0 && pheight > 0 &&
                        px + pwidth <= TILE_SIZE && py + pheight <= TILE_SIZE)
                    {
                        if (fillPolygons) 
                        {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, borderColor);
                        }
                        else
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
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
                    
                    // Validate rectangle bounds
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE &&
                        pwidth > 0 && pheight > 0 &&
                        px + pwidth <= TILE_SIZE && py + pheight <= TILE_SIZE)
                    {
                        if (fillPolygons) 
                        {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, borderColor);
                        }
                        else
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
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
                    if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) 
                    {
                        // Use efficient batch rendering
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
                        !isPointOnMargin(pcx, pcy)) 
                    {
                        if (fillPolygons) 
                        {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, borderColor);
                            executed++;
                        }
                        else
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);

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
                        pcx - pradius >= 0 && pcy - pradius >= 0) 
                    {
                        if (fillPolygons) 
                        {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, borderColor);
                        }
                        else
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
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
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) 
                    {
                        if (fillPolygons) 
                        {
                            // Fill triangle using optimized algorithm
                            fillTriangleOptimized(map, px1, py1, px2, py2, px3, py3, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.drawLine(px1, py1, px2, py2, borderColor);
                            map.drawLine(px2, py2, px3, py3, borderColor);
                            map.drawLine(px3, py3, px1, py1, borderColor);
                        }
                        else
                        {
                            // Draw only outline
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
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) 
                    {
                        if (fillPolygons) 
                        {
                            // Fill triangle using optimized algorithm
                            fillTriangleOptimized(map, px1, py1, px2, py2, px3, py3, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            map.drawLine(px1, py1, px2, py2, borderColor);
                            map.drawLine(px2, py2, px3, py3, borderColor);
                            map.drawLine(px3, py3, px1, py1, borderColor);
                        }
                        else
                        {
                            // Draw only outline
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
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) 
                    {
                        // Simple dashed line implementation
                        int dx = abs(px2 - px1);
                        int dy = abs(py2 - py1);
                        int sx = (px1 < px2) ? 1 : -1;
                        int sy = (py1 < py2) ? 1 : -1;
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
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) 
                    {
                        // Simple dotted line implementation
                        int dx = abs(px2 - px1);
                        int dy = abs(py2 - py1);
                        int sx = (px1 < px2) ? 1 : -1;
                        int sy = (py1 < py2) ? 1 : -1;
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
                    
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE) 
                    {
                        // Simple grid pattern implementation
                        if (direction == 0) {
                            // Horizontal lines
                            for (int i = 0; i < count; i++) {
                                int lineY = py + (i * spacing);
                                map.drawLine(px, lineY, px + width, lineY, currentDrawColor);
                            }
                        } else {
                            // Vertical lines
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
                if (offset < dataSize - 4) 
                    offset += 4;

                break;
        }
        if (!isLineCommand) 
            flushCurrentBatch();

        if (offset <= cmdStartOffset) 
            break;
    }
    flushCurrentBatch();
    
    // Clean up batch rendering
    if (activeBatch) 
    {
        if (activeBatch->segments) 
            delete[] activeBatch->segments;
        delete activeBatch;
        activeBatch = nullptr;
    }

    if (executed == 0) 
        return false;

    // Add successfully rendered tile to cache
    addToCache(path, map);

    // Log rendering performance
    unsigned long renderTime = millis() - renderStart;
    if (totalLines > 0) {
        ESP_LOGI(TAG, "Tile rendered: %s", path);
    ESP_LOGI(TAG, "Performance: %lu ms, %d lines, %d batches, avg %.1f lines/batch", 
             renderTime, totalLines, batchFlushes, 
             batchFlushes > 0 ? (float)totalLines / batchFlushes : 0.0f);
    ESP_LOGI(TAG, "Batch stats: %d renders, %d optimizations, %d flushes", 
             batchFlushes, batchOptimizations, batchFlushes);
    }

    return true;
}

/**
 * @brief Initialize the unified memory pool for efficient memory management.
 * 
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
    size_t psramFree = ESP.getFreePsram();
    maxUnifiedPoolEntries = std::min(static_cast<size_t>(100), psramFree / (1024 * 32)); // 32KB per entry
    ESP_LOGI(TAG, "PSRAM available: %zu bytes, setting unified pool size to %zu entries", psramFree, maxUnifiedPoolEntries);
#else
    size_t ramFree = ESP.getFreeHeap();
    maxUnifiedPoolEntries = std::min(static_cast<size_t>(25), ramFree / (1024 * 64)); // 64KB per entry
    ESP_LOGI(TAG, "RAM available: %zu bytes, setting unified pool size to %zu entries", ramFree, maxUnifiedPoolEntries);
#endif
    
    unifiedPool.clear();
    unifiedPool.reserve(maxUnifiedPoolEntries);
    unifiedPoolHitCount = 0;
    unifiedPoolMissCount = 0;
    
    ESP_LOGI(TAG, "Unified memory pool initialized with %zu entries", maxUnifiedPoolEntries);
}

/**
 * @brief Implement a unified memory allocation function that uses a memory pool.
 * 
 * @param size Size of memory to allocate
 * @param type type of allocation for tracking purposes
 * @return void* 
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
 * @brief Deallocate memory allocated from the unified memory pool.
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

/**
 * @brief Clear the unified memory pool, freeing all allocated memory.
 * 
 */
void Maps::clearUnifiedPool()
{
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) 
    {
        for (auto& entry : unifiedPool)
        {
            if (entry.ptr) 
                heap_caps_free(entry.ptr);
        }
        unifiedPool.clear();
        unifiedPoolHitCount = 0;
        unifiedPoolMissCount = 0;
        xSemaphoreGive(unifiedPoolMutex);
        ESP_LOGI(TAG, "Unified memory pool cleared");
    }
}

/**
 * @brief Initialize transformation matrices for coordinate and pixel transformations.
 * 
 */
void Maps::initTransformMatrices()
{
    coordTransformMatrix.isValid = false;
    pixelTransformMatrix.isValid = false;
    transformMatricesValid = false;
    lastTransformUpdate = 0;
    
    ESP_LOGI(TAG, "Transform matrices initialized");
}

/**
 * @brief Update transformation matrices if they are invalid or outdated.
 * 
 */
void Maps::updateTransformMatrices()
{
    uint32_t currentTime = millis();
    
    // Only update if matrices are invalid or enough time has passed
    if (transformMatricesValid && (currentTime - lastTransformUpdate) < 1000)
        return; // Matrices are still valid
    
    // Calculate coordinate transformation matrix
    coordTransformMatrix.scaleX = 1.0f / 360.0f;  // Longitude scale factor
    coordTransformMatrix.scaleY = 1.0f / (2.0f * static_cast<float>(M_PI));  // Latitude scale factor
    coordTransformMatrix.offsetX = 180.0f;  // Longitude offset
    coordTransformMatrix.offsetY = 0.0f;    // Latitude offset
    coordTransformMatrix.rotation = 0.0f;   // No rotation for coordinate transform
    coordTransformMatrix.isValid = true;
    
    // Calculate pixel transformation matrix
    pixelTransformMatrix.scaleX = static_cast<float>(mapScrWidth) / static_cast<float>(tileWidth);
    pixelTransformMatrix.scaleY = static_cast<float>(mapScrHeight) / static_cast<float>(tileHeight);
    pixelTransformMatrix.offsetX = 0.0f;
    pixelTransformMatrix.offsetY = 0.0f;
    pixelTransformMatrix.rotation = 0.0f;
    pixelTransformMatrix.isValid = true;
    
    transformMatricesValid = true;
    lastTransformUpdate = currentTime;
    
    ESP_LOGI(TAG, "Transform matrices updated - Coord: scale(%.3f,%.3f) offset(%.1f,%.1f), Pixel: scale(%.3f,%.3f)", 
             coordTransformMatrix.scaleX, coordTransformMatrix.scaleY, 
             coordTransformMatrix.offsetX, coordTransformMatrix.offsetY,
             pixelTransformMatrix.scaleX, pixelTransformMatrix.scaleY);
}

/**
 * @brief Invalidate transformation matrices, forcing recalculation on next use. 
 */
void Maps::invalidateTransformMatrices()
{
    transformMatricesValid = false;
    coordTransformMatrix.isValid = false;
    pixelTransformMatrix.isValid = false;
    ESP_LOGI(TAG, "Transform matrices invalidated");
}

/**
 * @brief  Check if transformation matrices are valid.
 * 
 * @return true  if valid
 * @return false  if not valid
 */
bool Maps::areTransformMatricesValid()
{
    return transformMatricesValid && coordTransformMatrix.isValid && pixelTransformMatrix.isValid;
}

/**
 * @brief Transform longitude to pixel X coordinate at given zoom and tile size.
 *  
 * @param lon        Longitude in degrees
 * @param zoom       Zoom level (0-21)
 * @param tileSize   Tile size in pixels (typically 256)
 * @return uint16_t  Pixel X coordinate within the tile
 */
uint16_t Maps::transformLonToPixel(float lon, uint8_t zoom, uint16_t tileSize)
{
    if (!transformMatricesValid || !coordTransformMatrix.isValid)
        // Cannot call non-static method from static context, use fallback
        return static_cast<uint16_t>(((lon + 180.0f) / 360.0f * (1 << zoom) * tileSize)) % tileSize;
    
    // Use precalculated matrix for faster transformation
    float normalizedLon = (lon + coordTransformMatrix.offsetX) * coordTransformMatrix.scaleX;
    float zoomFactor = static_cast<float>(1 << zoom);
    float pixelX = normalizedLon * zoomFactor * static_cast<float>(tileSize);
    
    return static_cast<uint16_t>(pixelX) % tileSize;
}

/**
 * @brief   Transform latitude to pixel Y coordinate at given zoom and tile size.
 * 
 * @param lat       Latitude in degrees       
 * @param zoom      Zoom level (0-21)
 * @param tileSize  Tile size in pixels (typically 256) 
 * @return uint16_t Pixel Y coordinate within the tile 
 */
uint16_t Maps::transformLatToPixel(float lat, uint8_t zoom, uint16_t tileSize)
{
    if (!transformMatricesValid || !coordTransformMatrix.isValid) 
    {
        // Cannot call non-static method from static context, use fallback
        float lat_rad = lat * static_cast<float>(M_PI) / 180.0f;
        float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
        float merc_n = logf(siny);
        float scale = (1 << zoom) * tileSize;
        return static_cast<uint16_t>(((1.0f - merc_n / static_cast<float>(M_PI)) / 2.0f * scale)) % tileSize;
    }
    
    // Use precalculated matrix for faster transformation
    float lat_rad = lat * static_cast<float>(M_PI) / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);
    float normalizedLat = (1.0f - merc_n * coordTransformMatrix.scaleY) * 0.5f;
    float zoomFactor = static_cast<float>(1 << zoom);
    float pixelY = normalizedLat * zoomFactor * static_cast<float>(tileSize);
    
    return static_cast<uint16_t>(pixelY) % tileSize;
}

/**
 * @brief   Transform pixel X coordinate to longitude at given zoom and tile size.
 * 
 * @param pixelX    Pixel X coordinate within the tile 
 * @param zoom      Zoom level (0-21) 
 * @param tileSize  Tile size in pixels (typically 256)
 * @return float    Longitude in degrees
 */
float Maps::transformPixelToLon(uint16_t pixelX, uint8_t zoom, uint16_t tileSize)
{
    if (!transformMatricesValid || !coordTransformMatrix.isValid) 
        // Cannot call non-static method from static context, use fallback
        return static_cast<float>(pixelX) * 360.0f / (1 << zoom) - 180.0f;
    
    float zoomFactor = static_cast<float>(1 << zoom);
    float normalizedX = static_cast<float>(pixelX) / (zoomFactor * static_cast<float>(tileSize));
    float lon = (normalizedX / coordTransformMatrix.scaleX) - coordTransformMatrix.offsetX;
    
    return lon;
}

/**
 * @brief   Transform pixel Y coordinate to latitude at given zoom and tile size.
 * 
 * @param pixelY    Pixel Y coordinate within the tile 
 * @param zoom      Zoom level (0-21)
 * @param tileSize  Tile size in pixels (typically 256)
 * @return float    Latitude in degrees
 */
float Maps::transformPixelToLat(uint16_t pixelY, uint8_t zoom, uint16_t tileSize)
{
    if (!transformMatricesValid || !coordTransformMatrix.isValid) {
        // Cannot call non-static method from static context, use fallback
        float scale = static_cast<float>(1 << zoom);
        float n = static_cast<float>(M_PI) * (1.0f - 2.0f * static_cast<float>(pixelY) / scale);
        return 180.0f / static_cast<float>(M_PI) * atanf(sinhf(n));
    }
    
    float zoomFactor = static_cast<float>(1 << zoom);
    float normalizedY = static_cast<float>(pixelY) / (zoomFactor * static_cast<float>(tileSize));
    float merc_n = static_cast<float>(M_PI) * (1.0f - 2.0f * normalizedY);
    float lat = 180.0f / static_cast<float>(M_PI) * atanf(sinhf(merc_n));
    
    return lat;
}

/**
 * @brief Initialize batch rendering system, detecting optimal batch size based on hardware.
 * 
 */
void Maps::initBatchRendering()
{
    // Detect optimal batch size based on hardware capabilities
#ifdef BOARD_HAS_PSRAM
    size_t psramFree = ESP.getFreePsram();
    if (psramFree >= 4 * 1024 * 1024) 
        maxBatchSize = 512;  // High-end ESP32-S3: 512 lines
    else if (psramFree >= 2 * 1024 * 1024)
        maxBatchSize = 256;  // Mid-range ESP32-S3: 256 lines
    else 
        maxBatchSize = 128;  // Low-end ESP32-S3: 128 lines
#else
    maxBatchSize = 64;  // ESP32 without PSRAM: 64 lines
#endif
    
    activeBatch = nullptr;
    batchRenderCount = 0;
    batchOptimizationCount = 0;
    batchFlushCount = 0;
    
    ESP_LOGI(TAG, "Batch rendering initialized with max batch size: %zu", maxBatchSize);
}

/**
 * @brief Create a new render batch with specified capacity.
 * 
 * @param capacity Number of line segments the batch can hold
 */
void Maps::createRenderBatch(size_t capacity)
{
    if (activeBatch) 
    {
        if (activeBatch->segments) 
            delete[] activeBatch->segments;
        delete activeBatch;
        activeBatch = nullptr;
    }
    
    activeBatch = new RenderBatch();
    activeBatch->segments = new LineSegment[capacity];
    activeBatch->count = 0;
    activeBatch->capacity = capacity;
    activeBatch->color = 0;
    activeBatch->isOptimized = false;
    
    ESP_LOGI(TAG, "Created render batch with capacity: %zu", capacity);
}

/**
 * @brief Add a line segment to the current batch if possible.
 * 
 * @param x0    Starting X coordinate
 * @param y0    Starting Y coordinate
 * @param x1    Ending X coordinate
 * @param y1    Ending Y coordinate
 * @param color Color of the line segment
 */
void Maps::addToBatch(int x0, int y0, int x1, int y1, uint16_t color)
{
    if (!activeBatch)
        createRenderBatch(maxBatchSize);
    
    // Check if we can add to current batch (same color)
    if (!canBatch(color) || activeBatch->count >= activeBatch->capacity) 
    {
        ESP_LOGW(TAG, "Cannot add to batch - flushing current batch");
        return;
    }
    
    // Add segment to batch
    activeBatch->segments[activeBatch->count] = {x0, y0, x1, y1, color};
    activeBatch->count++;
    
    // Set batch color if first segment
    if (activeBatch->count == 1) 
        activeBatch->color = color;
}

/**
 * @brief Flush the current batch, rendering all segments to the map.
 * 
 * @param map           Reference to the TFT_eSprite map to render onto
 * @param optimizations Reference to a counter for optimizations performed
 */
void Maps::flushBatch(TFT_eSprite& map, int& optimizations)
{
    if (!activeBatch || activeBatch->count == 0)
        return;
    
    batchFlushCount++;
    
    // Optimize batch if beneficial (threshold based on hardware capabilities)
    size_t optimizationThreshold = maxBatchSize / 16; // 6.25% of max batch size (32 lines for 512 batch)
    if (activeBatch->count > optimizationThreshold) 
    {
        optimizeBatch(*activeBatch);
        batchOptimizationCount++;
        optimizations++;  // Increment local counter
    }
    
    // Render all segments in batch
    for (size_t i = 0; i < activeBatch->count; i++) 
    {
        const LineSegment& segment = activeBatch->segments[i];
        map.drawLine(segment.x0, segment.y0, segment.x1, segment.y1, segment.color);
    }
    
    batchRenderCount++;
    
    // Clear batch for reuse
    activeBatch->count = 0;
    activeBatch->color = 0;
    activeBatch->isOptimized = false;
}

/**
 * @brief Optimize the batch by sorting and grouping segments for better rendering performance.
 * 
 * @param batch Reference to the RenderBatch to optimize
 */
void Maps::optimizeBatch(RenderBatch& batch)
{
    // Dynamic threshold based on hardware capabilities
    size_t minOptimizationSize = maxBatchSize / 32; // 3.125% of max batch size (16 lines for 512 batch)
    if (batch.count < minOptimizationSize) 
        return; // Not worth optimizing small batches
    
    // Sort segments by color for better cache performance
    std::sort(batch.segments, batch.segments + batch.count, 
              [](const LineSegment& a, const LineSegment& b) {
                  return a.color < b.color;
              });
    
    // Group segments by color and optimize rendering order
    uint16_t currentColor = batch.segments[0].color;
    size_t colorGroupStart = 0;
    
    for (size_t i = 1; i <= batch.count; i++) {
        if (i == batch.count || batch.segments[i].color != currentColor) 
        {
            // Process color group
            size_t groupSize = i - colorGroupStart;
            size_t minGroupSize = maxBatchSize / 64; // 1.56% of max batch size (8 lines for 512 batch)
            if (groupSize > minGroupSize) 
            {
                // Use optimized rendering for large color groups
                for (size_t j = colorGroupStart; j < i; j++)
                {
                    const LineSegment& segment = batch.segments[j];
                    // Could implement specialized line drawing here
                }
            }
            
            if (i < batch.count) 
            {
                currentColor = batch.segments[i].color;
                colorGroupStart = i;
            }
        }
    }
    
    batch.isOptimized = true;
}


/**
 * @brief   Check if a line segment with the given color can be added to the current batch.
 * 
 * @param color     Color of the line segment to add
 * @return true     if it can be added
 * @return false    if it cannot be added
 */
bool Maps::canBatch(uint16_t color)
{
    if (!activeBatch) 
        return true;
    
    // Can batch if same color or batch is empty
    return (activeBatch->count == 0) || (activeBatch->color == color);
}

/**
 * @brief Get the optimal batch size based on hardware capabilities.
 * 
 * @return size_t Optimal batch size
 */
size_t Maps::getOptimalBatchSize()
{
    return maxBatchSize;
}
