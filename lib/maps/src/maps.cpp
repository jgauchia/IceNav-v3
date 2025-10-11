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
// fillPolygons is a non-static member variable, will be initialized in constructor

// Tile cache system static variables
std::vector<Maps::CachedTile> Maps::tileCache;
size_t Maps::maxCachedTiles = 0;
uint32_t Maps::cacheAccessCounter = 0;

// Background preload system static variables
std::vector<Maps::PreloadTask> Maps::preloadQueue;
TaskHandle_t Maps::preloadTaskHandle = nullptr;
SemaphoreHandle_t Maps::preloadMutex = nullptr;
bool Maps::preloadSystemActive = false;

// Memory pool system static variables
std::vector<Maps::MemoryPoolEntry> Maps::memoryPool;
SemaphoreHandle_t Maps::memoryPoolMutex = nullptr;
size_t Maps::maxPoolEntries = 0;
uint32_t Maps::poolAllocationCount = 0;
uint32_t Maps::poolHitCount = 0;
uint32_t Maps::poolMissCount = 0;

// Advanced memory pools static variables
Maps::PointPool Maps::pointPool;
Maps::CommandPool Maps::commandPool;
Maps::CoordsPool Maps::coordsPool;
Maps::FeaturePool Maps::featurePool;
Maps::LineSegmentPool Maps::lineSegmentPool;
Maps::CoordArrayPool Maps::coordArrayPool;
SemaphoreHandle_t Maps::advancedPoolMutex = nullptr;

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

// Initialize static variables for layer rendering
std::vector<std::vector<Maps::RenderCommand>> Maps::layerCommands(LAYER_COUNT);
bool Maps::layerRenderingEnabled = false;
uint32_t Maps::layerRenderCount = 0;

// Adaptive batching based on hardware capabilities
#ifdef BOARD_HAS_PSRAM
    #define LINE_BATCH_SIZE_BASE 128  // Base size for ESP32-S3
#else
    #define LINE_BATCH_SIZE_BASE 32   // Smaller size for ESP32 without PSRAM
#endif

// Calculate optimal batch size based on available memory
static size_t getOptimalBatchSize() {
    static size_t optimalSize = 0;
    if (optimalSize == 0) {
#ifdef BOARD_HAS_PSRAM
        size_t psramFree = ESP.getFreePsram();
        if (psramFree >= 4 * 1024 * 1024) {
            optimalSize = 256;  // High-end ESP32-S3: 256 lines
        } else if (psramFree >= 2 * 1024 * 1024) {
            optimalSize = 128;  // Mid-range ESP32-S3: 128 lines
        } else {
            optimalSize = 64;   // Low-end ESP32-S3: 64 lines
        }
#else
        optimalSize = 32;  // ESP32 without PSRAM: 32 lines
#endif
        ESP_LOGI(TAG, "Optimal batch size: %zu lines", optimalSize);
    }
    return optimalSize;
}

#define LINE_BATCH_SIZE getOptimalBatchSize()

/**
 * @brief Map Class constructor
 */
Maps::Maps() : fillPolygons(true) 
{
    ESP_LOGI(TAG, "Maps constructor called - initializing pools...");
    
    // Initialize advanced memory pools in constructor
    initAdvancedPools();
    initUnifiedPool();
    initMemoryMonitoring();
    
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
	
	// Initialize background preload system
	initBackgroundPreload();
	
	// Initialize memory pool system
	initMemoryPool();
	
	// Initialize polygon optimizations
	initPolygonOptimizations();
	
	// Initialize layer rendering system
	initLayerRendering();
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
	
	// Stop background preload system
	disableBackgroundPreload();
	
	// Clear memory pool
	clearMemoryPool();
	
	// Clear layer commands
	clearLayerCommands();
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
	if (Maps::zoomLevel != zoom && Maps::zoomLevel != 0) {
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
			
			// Trigger background preload of adjacent tiles
			if (preloadSystemActive) {
				triggerPreload(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);
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
		if (mapSet.vectorMap) {
			foundTile = getCachedTile(Maps::roundMapTile.file, preloadSprite, offsetX, offsetY);
			if (foundTile) {
				ESP_LOGI(TAG, "Tile found in cache: %s", Maps::roundMapTile.file);
			}
		}
		
		// If not in cache, try to load from file
		if (!foundTile) {
			if (mapSet.vectorMap) {
				ESP_LOGI(TAG, "Rendering tile to cache: %s", Maps::roundMapTile.file);
				// Create a temporary sprite for rendering to cache
				TFT_eSprite tempSprite = TFT_eSprite(&tft);
				tempSprite.createSprite(tileSize, tileSize);
				
				// Render tile to temporary sprite (this will cache it)
				foundTile = renderTile(Maps::roundMapTile.file, 0, 0, tempSprite);
				
				if (foundTile) {
					// Copy from temporary sprite to preload sprite
					preloadSprite.pushImage(offsetX, offsetY, tileSize, tileSize, tempSprite.frameBuffer(0));
					ESP_LOGI(TAG, "Tile rendered and cached: %s", Maps::roundMapTile.file);
				}
				
				tempSprite.deleteSprite();
			} else {
				foundTile = preloadSprite.drawPngFile(Maps::roundMapTile.file, offsetX, offsetY);
			}
		}

		if (!foundTile)
		{
			preloadSprite.fillRect(offsetX, offsetY, tileSize, tileSize, TFT_LIGHTGREY);
		}
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
    
    // Read 4-byte header for number of colors (like tile_viewer.py)
    uint32_t numColors;
    if (fread(&numColors, 4, 1, f) != 1) {
        fclose(f);
        return false;
    }
    
    // Read RGB888 colors (3 bytes per color) and convert to RGB332
    uint8_t rgb888[3];
    PALETTE_SIZE = 0;
    
    for (uint32_t i = 0; i < numColors && i < 256; i++) {
        if (fread(rgb888, 3, 1, f) == 1) {
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
    if (idx < PALETTE_SIZE) {
        return PALETTE[idx];
    }
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
    // Convert RGB332 to RGB565 using same method as tile_generator.py write_palette_bin
    uint8_t r = (color & 0xE0);
    uint8_t g = (color & 0x1C) << 3;
    uint8_t b = (color & 0x03) << 6;
    
    // Convert to RGB565 format (reverse of RGB888 expansion in tile_generator.py)
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
    // Use unified pool directly
    int *xints = nullptr;
    
    // Use unified pool directly
    xints = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
    if (!xints) {
        ESP_LOGW(TAG, "fillPolygonGeneral: Unified pool allocation failed");
        return;
    }
    if (!unifiedPoolLogged) {
        ESP_LOGI(TAG, "fillPolygonGeneral: Using UNIFIED POOL for coordinate arrays");
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
    
    // Always use unified deallocation since we use unified pool directly
    unifiedDealloc(xints);
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
void Maps::drawFilledCircleWithBorder(TFT_eSprite& map, int x, int y, int r, uint16_t fill, uint16_t border, bool fillShape)
{
    if (fillShape) 
    {
        map.fillCircle(x, y, r, fill);
        map.drawCircle(x, y, r, border);
    }
    else 
        map.drawCircle(x, y, r, fill);
}

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
void Maps::drawFilledRectWithBorder(TFT_eSprite& map, int x, int y, int w, int h, uint16_t fill, uint16_t border, bool fillShape)
{
    if (fillShape) 
    {
        map.fillRect(x, y, w, h, fill);
        map.drawRect(x, y, w, h, border);
    }
    else 
        map.drawRect(x, y, w, h, fill);
}

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
void Maps::drawDashedLine(TFT_eSprite& map, int x1, int y1, int x2, int y2, int dashLength, int gapLength, uint16_t color)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int currentX = x1;
    int currentY = y1;
    int segmentLength = 0;
    bool drawingDash = true;
    
    while (true) {
        if (drawingDash) {
            map.drawPixel(currentX, currentY, color);
            segmentLength++;
            
            if (segmentLength >= dashLength) {
                drawingDash = false;
                segmentLength = 0;
            }
        } else {
            segmentLength++;
            if (segmentLength >= gapLength) {
                drawingDash = true;
                segmentLength = 0;
            }
        }
        
        if (currentX == x2 && currentY == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            currentX += sx;
        }
        if (e2 < dx) {
            err += dx;
            currentY += sy;
        }
    }
}

/**
 * @brief Draw a predicted line based on pattern
 *
 * @details Draws a predicted line using pattern-based prediction for road networks.
 *
 * @param map The TFT_eSprite object where the line will be drawn.
 * @param startX, startY Starting point coordinates.
 * @param patternType Type of pattern (0=straight, 1=curved, 2=zigzag).
 * @param length Length of the predicted line.
 * @param color The color to draw the line with (in RGB565 format).
 */
void Maps::drawPredictedLine(TFT_eSprite& map, int startX, int startY, int patternType, int length, uint16_t color)
{
    int endX = startX;
    int endY = startY;
    
    switch (patternType) {
        case 0: // Straight line
            endX = startX + length;
            endY = startY;
            break;
        case 1: // Curved line
            endX = startX + length;
            endY = startY + (length / 4); // Gentle curve
            break;
        case 2: // Zigzag line
            endX = startX + length;
            endY = startY + (length / 8); // Small zigzag
            break;
        default:
            endX = startX + length;
            endY = startY;
            break;
    }
    
    // Draw the predicted line
    map.drawLine(startX, startY, endX, endY, color);
}

/**
 * @brief Draw a highway segment with width
 *
 * @details Draws a highway segment using multiple parallel lines to simulate width.
 *
 * @param map The TFT_eSprite object where the segment will be drawn.
 * @param startX, startY Starting point coordinates.
 * @param endX, endY Ending point coordinates.
 * @param width Width of the highway segment.
 * @param color The color to draw the segment with (in RGB565 format).
 */
void Maps::drawHighwaySegment(TFT_eSprite& map, int startX, int startY, int endX, int endY, int width, uint16_t color)
{
    if (width <= 1) {
        // Draw as simple line if width is 1 or less
        map.drawLine(startX, startY, endX, endY, color);
        return;
    }
    
    // Calculate perpendicular direction for width
    int dx = endX - startX;
    int dy = endY - startY;
    int length = sqrt(dx * dx + dy * dy);
    
    if (length == 0) return; // Avoid division by zero
    
    // Normalize and rotate 90 degrees for perpendicular
    float perpX = -dy / (float)length;
    float perpY = dx / (float)length;
    
    // Calculate half width offset
    int halfWidth = width / 2;
    
    // Draw multiple parallel lines to simulate width
    for (int i = -halfWidth; i <= halfWidth; i++) {
        int offsetX = (int)(perpX * i);
        int offsetY = (int)(perpY * i);
        
        map.drawLine(startX + offsetX, startY + offsetY, 
                    endX + offsetX, endY + offsetY, color);
    }
}

/**
 * @brief Draw a block pattern for urban areas
 *
 * @details Draws a pattern of rectangular blocks to represent urban areas.
 *
 * @param map The TFT_eSprite object where the pattern will be drawn.
 * @param x, y Starting coordinates.
 * @param blockSize Size of each block.
 * @param spacing Spacing between blocks.
 * @param count Number of blocks to draw.
 * @param color The color to draw the blocks with (in RGB565 format).
 */
void Maps::drawBlockPattern(TFT_eSprite& map, int x, int y, int blockSize, int spacing, int count, uint16_t color)
{
    int currentX = x;
    int currentY = y;
    
    for (int i = 0; i < count; i++) {
        // Draw block rectangle
        map.drawRect(currentX, currentY, blockSize, blockSize, color);
        
        // Move to next position
        currentX += blockSize + spacing;
        
        // Check if we need to wrap to next row
        if (currentX + blockSize > map.width()) {
            currentX = x;
            currentY += blockSize + spacing;
        }
        
        // Stop if we go beyond the map bounds
        if (currentY + blockSize > map.height()) {
            break;
        }
    }
}

/**
 * @brief Draw a dotted line between two points
 *
 * @details Draws a dotted line using Bresenham-like algorithm with configurable dot spacing.
 *
 * @param map The TFT_eSprite object where the line will be drawn.
 * @param x1, y1 Starting point coordinates.
 * @param x2, y2 Ending point coordinates.
 * @param dotSpacing Distance between dots.
 * @param color The color to draw the line with (in RGB565 format).
 */
void Maps::drawDottedLine(TFT_eSprite& map, int x1, int y1, int x2, int y2, int dotSpacing, uint16_t color)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int currentX = x1;
    int currentY = y1;
    int dotCounter = 0;
    
    while (true) {
        if (dotCounter % dotSpacing == 0) {
            map.drawPixel(currentX, currentY, color);
        }
        dotCounter++;
        
        if (currentX == x2 && currentY == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            currentX += sx;
        }
        if (e2 < dx) {
            err += dx;
            currentY += sy;
        }
    }
}

/**
 * @brief Draw a grid pattern (horizontal or vertical lines)
 *
 * @details Draws a series of parallel lines to create a grid pattern, commonly used for tile borders.
 *
 * @param map The TFT_eSprite object where the pattern will be drawn.
 * @param x, y Starting position coordinates.
 * @param width Length of each line in the pattern.
 * @param spacing Distance between lines.
 * @param count Number of lines to draw.
 * @param direction 0 for horizontal lines, 1 for vertical lines.
 * @param color The color to draw the lines with (in RGB565 format).
 */
void Maps::drawGridPattern(TFT_eSprite& map, int x, int y, int width, int spacing, int count, int direction, uint16_t color)
{
    if (direction == 0) {
        // Horizontal lines
        for (int i = 0; i < count; i++) {
            int lineY = y + (i * spacing);
            map.drawLine(x, lineY, x + width, lineY, color);
        }
    } else {
        // Vertical lines
        for (int i = 0; i < count; i++) {
            int lineX = x + (i * spacing);
            map.drawLine(lineX, y, lineX, y + width, color);
        }
    }
}

/**
* @brief Allocates a buffer for a specified number of elements of type T using memory pool.
*
* @details This template function attempts to allocate memory for an array of numElements of type T using the memory pool.
*          If pool allocation fails, it falls back to standard heap allocation with PSRAM preference.
*          The function returns a pointer to the allocated memory, or nullptr if the allocation fails.
*
* @tparam T The type of elements to allocate.
* @param numElements The number of elements to allocate.
* @return A pointer to the allocated buffer, or nullptr if allocation fails.
*/
template<typename T>
T* Maps::allocBuffer(size_t numElements) 
{
    size_t totalSize = numElements * sizeof(T);
    
    // Try memory pool first
    void* ptr = poolAllocate(totalSize);
    if (ptr) {
        poolHitCount++;
        totalMemoryAllocations++;
        return static_cast<T*>(ptr);
    }
    
    // Fallback to standard allocation
    poolMissCount++;
    totalMemoryAllocations++;
#ifdef BOARD_HAS_PSRAM
    ptr = heap_caps_malloc(totalSize, MALLOC_CAP_SPIRAM);
    if (ptr) return static_cast<T*>(ptr);
#endif
    ptr = heap_caps_malloc(totalSize, MALLOC_CAP_8BIT);
    return static_cast<T*>(ptr);
}

/**
 * @brief Initialize tile cache system
 *
 * @details Initializes the tile cache system by detecting hardware capabilities and setting up the cache structure.
 */
void Maps::initTileCache()
{
    detectHardwareCapabilities();
    tileCache.clear();
    tileCache.reserve(maxCachedTiles);
    cacheAccessCounter = 0;
    ESP_LOGI(TAG, "Tile cache initialized with %zu tiles capacity", maxCachedTiles);
}

/**
 * @brief Detect hardware capabilities and set cache size
 *
 * @details Automatically detects available memory (PSRAM/RAM) and sets the appropriate cache size.
 */
void Maps::detectHardwareCapabilities()
{
    maxCachedTiles = 0; // Default: no cache
    
#ifdef BOARD_HAS_PSRAM
    // Check PSRAM size and availability
    size_t psramSize = ESP.getPsramSize();
    size_t psramFree = ESP.getFreePsram();
    size_t psramUsed = psramSize - psramFree;
    
    ESP_LOGI(TAG, "PSRAM total: %zu bytes (%.2f MB)", psramSize, psramSize / (1024.0 * 1024.0));
    ESP_LOGI(TAG, "PSRAM free: %zu bytes (%.2f MB)", psramFree, psramFree / (1024.0 * 1024.0));
    ESP_LOGI(TAG, "PSRAM used: %zu bytes (%.2f MB)", psramUsed, psramUsed / (1024.0 * 1024.0));
    
    // Calculate available memory for cache (leave 50% free for other operations)
    size_t availableForCache = psramFree * 0.5;
    size_t tileMemorySize = tileWidth * tileHeight * 2; // RGB565 = 2 bytes per pixel
    
    ESP_LOGI(TAG, "Available for cache: %zu bytes (%.2f MB)", availableForCache, availableForCache / (1024.0 * 1024.0));
    ESP_LOGI(TAG, "Memory per tile: %zu bytes (%.2f MB)", tileMemorySize, tileMemorySize / (1024.0 * 1024.0));
    
    // Calculate max tiles based on available memory
    maxCachedTiles = availableForCache / tileMemorySize;
    
    // Apply reasonable limits
    if (maxCachedTiles > 4) maxCachedTiles = 4; // Max 4 tiles regardless of memory
    if (maxCachedTiles < 1) maxCachedTiles = 0; // Disable if less than 1 tile
    
    ESP_LOGI(TAG, "Calculated cache capacity: %zu tiles", maxCachedTiles);
    
    if (maxCachedTiles >= 3) {
        ESP_LOGI(TAG, "High-end ESP32-S3 detected: %zu tiles cache enabled", maxCachedTiles);
    } else if (maxCachedTiles >= 2) {
        ESP_LOGI(TAG, "Mid-range ESP32-S3 detected: %zu tiles cache enabled", maxCachedTiles);
    } else if (maxCachedTiles >= 1) {
        ESP_LOGI(TAG, "Low-end ESP32-S3 detected: %zu tiles cache enabled", maxCachedTiles);
    } else {
        ESP_LOGI(TAG, "Insufficient PSRAM for tile cache - disabled");
    }
#else
    // ESP32 without PSRAM: no cache (memory too limited)
    ESP_LOGI(TAG, "ESP32 without PSRAM detected: tile cache disabled");
#endif
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
    
    for (auto& cachedTile : tileCache) {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) {
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
    for (auto& cachedTile : tileCache) {
        if (cachedTile.isValid && cachedTile.tileHash == tileHash) {
            cachedTile.lastAccess = ++cacheAccessCounter;
            return; // Already cached
        }
    }
    
    // Need to add new entry
    if (tileCache.size() >= maxCachedTiles) {
        evictLRUTile(); // Make room
    }
    
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
    
    for (auto it = tileCache.begin(); it != tileCache.end(); ++it) {
        if (it->lastAccess < oldestAccess) {
            oldestAccess = it->lastAccess;
            lruIt = it;
        }
    }
    
    // Free the sprite memory
    if (lruIt->sprite) {
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
    for (auto& cachedTile : tileCache) {
        if (cachedTile.sprite) {
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
    for (const auto& cachedTile : tileCache) {
        if (cachedTile.isValid && cachedTile.sprite) {
            memoryUsage += tileWidth * tileHeight * 2; // RGB565 = 2 bytes per pixel
        }
    }
    return memoryUsage;
}

/**
 * @brief Initialize tile cache system (public method)
 *
 * @details Public method to initialize the tile cache system.
 */
void Maps::initializeTileCache()
{
    initTileCache();
}

/**
 * @brief Print cache statistics for debugging
 *
 * @details Prints useful cache statistics for debugging and monitoring.
 */
void Maps::printCacheStats()
{
    ESP_LOGI(TAG, "=== Tile Cache Statistics ===");
    ESP_LOGI(TAG, "Max cached tiles: %zu", maxCachedTiles);
    ESP_LOGI(TAG, "Current cached tiles: %zu", tileCache.size());
    ESP_LOGI(TAG, "Cache memory usage: %zu bytes", getCacheMemoryUsage());
    ESP_LOGI(TAG, "Cache access counter: %u", cacheAccessCounter);
    
    for (size_t i = 0; i < tileCache.size(); ++i) {
        const auto& tile = tileCache[i];
        ESP_LOGI(TAG, "Cache[%zu]: %s (hash: %u, access: %u)", 
                 i, tile.filePath, tile.tileHash, tile.lastAccess);
    }
    ESP_LOGI(TAG, "=============================");
}

/**
 * @brief Initialize background preload system
 *
 * @details Initializes the background preload system with FreeRTOS task and mutex.
 */
void Maps::initBackgroundPreload()
{
    if (preloadMutex == nullptr) {
        preloadMutex = xSemaphoreCreateMutex();
        if (preloadMutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create preload mutex");
            return;
        }
    }
    
    preloadQueue.clear();
    preloadSystemActive = false;
    ESP_LOGI(TAG, "Background preload system initialized");
}

/**
 * @brief Start FreeRTOS preload task
 *
 * @details Creates and starts the FreeRTOS task for background tile preloading.
 */
void Maps::startPreloadTask()
{
    if (preloadTaskHandle != nullptr) {
        ESP_LOGI(TAG, "Preload task already running");
        return;
    }
    
    BaseType_t result = xTaskCreate(
        preloadTaskFunction,
        "TilePreload",
        4096,  // Stack size
        nullptr,
        2,     // Priority (lower than main tasks)
        &preloadTaskHandle
    );
    
    if (result == pdPASS) {
        preloadSystemActive = true;
        ESP_LOGI(TAG, "Background preload task started");
    } else {
        ESP_LOGE(TAG, "Failed to create preload task");
    }
}

/**
 * @brief Stop FreeRTOS preload task
 *
 * @details Stops and deletes the FreeRTOS preload task.
 */
void Maps::stopPreloadTask()
{
    if (preloadTaskHandle != nullptr) {
        preloadSystemActive = false;
        vTaskDelete(preloadTaskHandle);
        preloadTaskHandle = nullptr;
        ESP_LOGI(TAG, "Background preload task stopped");
    }
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
void Maps::addToPreloadQueue(const char* filePath, int16_t tileX, int16_t tileY, uint8_t zoom)
{
    if (!preloadSystemActive || !preloadMutex) return;
    
    if (xSemaphoreTake(preloadMutex, portMAX_DELAY) == pdTRUE) {
        // Check if tile is already in queue
        bool alreadyQueued = false;
        for (const auto& task : preloadQueue) {
            if (strcmp(task.filePath, filePath) == 0) {
                alreadyQueued = true;
                break;
            }
        }
        
        if (!alreadyQueued && preloadQueue.size() < 8) { // Limit queue size
            PreloadTask newTask;
            strncpy(newTask.filePath, filePath, sizeof(newTask.filePath) - 1);
            newTask.filePath[sizeof(newTask.filePath) - 1] = '\0';
            newTask.tileX = tileX;
            newTask.tileY = tileY;
            newTask.zoom = zoom;
            newTask.isActive = false;
            newTask.isCompleted = false;
            
            preloadQueue.push_back(newTask);
            ESP_LOGI(TAG, "Added to preload queue: %s", filePath);
        }
        
        xSemaphoreGive(preloadMutex);
    }
}

/**
 * @brief Process preload queue in background
 *
 * @details Processes tiles in the preload queue, rendering them to cache.
 */
void Maps::processPreloadQueue()
{
    if (!preloadSystemActive || !preloadMutex) return;
    
    if (xSemaphoreTake(preloadMutex, 0) == pdTRUE) {
        for (auto& task : preloadQueue) {
            if (!task.isActive && !task.isCompleted) {
                task.isActive = true;
                
                // Create temporary sprite for rendering
                TFT_eSprite tempSprite = TFT_eSprite(&tft);
                tempSprite.createSprite(tileWidth, tileHeight);
                
                // Render tile to cache (only for vector maps)
                bool success = false;
                if (mapSet.vectorMap) {
                    // Create a temporary Maps instance to call non-static method
                    Maps tempMaps;
                    success = tempMaps.renderTile(task.filePath, 0, 0, tempSprite);
                } else {
                    // Skip PNG tiles in background preload
                    success = false;
                }
                
                tempSprite.deleteSprite();
                
                task.isCompleted = success;
                task.isActive = false;
                
                if (success) {
                    ESP_LOGI(TAG, "Background preloaded: %s", task.filePath);
                }
                
                // Yield to other tasks
                vTaskDelay(1);
                break; // Process one tile per call
            }
        }
        
        // Remove completed tasks
        preloadQueue.erase(
            std::remove_if(preloadQueue.begin(), preloadQueue.end(),
                [](const PreloadTask& task) { return task.isCompleted; }),
            preloadQueue.end()
        );
        
        xSemaphoreGive(preloadMutex);
    }
}

/**
 * @brief FreeRTOS task function for background preloading
 *
 * @details The main function for the FreeRTOS preload task.
 *
 * @param parameter Task parameter (unused).
 */
void Maps::preloadTaskFunction(void* parameter)
{
    ESP_LOGI(TAG, "Background preload task started");
    
    while (preloadSystemActive) {
        processPreloadQueue();
        vTaskDelay(10); // Small delay to prevent CPU hogging
    }
    
    ESP_LOGI(TAG, "Background preload task ended");
    vTaskDelete(nullptr);
}

/**
 * @brief Preload tiles around current position
 *
 * @details Preloads tiles in a 3x3 grid around the current position.
 *
 * @param centerX The X coordinate of the center tile.
 * @param centerY The Y coordinate of the center tile.
 * @param zoom The zoom level.
 */
void Maps::preloadAdjacentTiles(int16_t centerX, int16_t centerY, uint8_t zoom)
{
    if (!preloadSystemActive) return;
    
    // Preload 3x3 grid around current position
    for (int8_t dy = -1; dy <= 1; dy++) {
        for (int8_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue; // Skip center tile
            
            int16_t tileX = centerX + dx;
            int16_t tileY = centerY + dy;
            
            MapTile tile = getMapTile(
                currentMapTile.lon, currentMapTile.lat,
                zoom, dx, dy
            );
            
            addToPreloadQueue(tile.file, tileX, tileY, zoom);
        }
    }
}

/**
 * @brief Enable background preload system (public method)
 *
 * @details Public method to enable the background preload system.
 */
void Maps::enableBackgroundPreload()
{
    initBackgroundPreload();
    startPreloadTask();
}

/**
 * @brief Disable background preload system (public method)
 *
 * @details Public method to disable the background preload system.
 */
void Maps::disableBackgroundPreload()
{
    stopPreloadTask();
    if (preloadMutex) {
        vSemaphoreDelete(preloadMutex);
        preloadMutex = nullptr;
    }
}

/**
 * @brief Trigger preload of adjacent tiles (public method)
 *
 * @details Public method to trigger preload of tiles around current position.
 *
 * @param centerX The X coordinate of the center tile.
 * @param centerY The Y coordinate of the center tile.
 * @param zoom The zoom level.
 */
void Maps::triggerPreload(int16_t centerX, int16_t centerY, uint8_t zoom)
{
    preloadAdjacentTiles(centerX, centerY, zoom);
}

/**
 * @brief Initialize advanced memory pools
 *
 * @details Initializes the advanced memory pool system with object-specific pools.
 */
void Maps::initAdvancedPools()
{
    ESP_LOGI(TAG, "Starting advanced pools initialization...");
    
    detectAdvancedPoolCapabilities();
    
    if (advancedPoolMutex == nullptr) {
        advancedPoolMutex = xSemaphoreCreateMutex();
        if (advancedPoolMutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create advanced pool mutex");
            return;
        }
        ESP_LOGI(TAG, "Advanced pool mutex created");
    }
    
    // Initialize point pool
    pointPool.points.clear();
    pointPool.points.reserve(pointPool.maxSize);
    pointPool.hits = 0;
    pointPool.misses = 0;
    
    // Initialize command pool
    commandPool.commands.clear();
    commandPool.commands.reserve(commandPool.maxSize);
    commandPool.hits = 0;
    commandPool.misses = 0;
    
    // Initialize coords pool
    coordsPool.coords.clear();
    coordsPool.coords.reserve(coordsPool.maxSize);
    coordsPool.hits = 0;
    coordsPool.misses = 0;
    
    // Initialize feature pool
    featurePool.features.clear();
    featurePool.features.reserve(featurePool.maxSize);
    featurePool.hits = 0;
    featurePool.misses = 0;
    
    // Initialize line segment pool with pre-allocated batches
    lineSegmentPool.lineBatches.clear();
    lineSegmentPool.lineBatches.reserve(lineSegmentPool.maxSize);
    
    // Pre-allocate line segment batches
    ESP_LOGI(TAG, "Pre-allocating %zu line segment batches...", lineSegmentPool.maxSize);
    for (size_t i = 0; i < lineSegmentPool.maxSize; i++) {
        std::vector<LineSegment> batch;
        batch.reserve(256); // Pre-allocate space for 256 line segments
        lineSegmentPool.lineBatches.push_back(batch);
    }
    ESP_LOGI(TAG, "Pre-allocated %zu line segment batches", lineSegmentPool.lineBatches.size());
    
    lineSegmentPool.hits = 0;
    lineSegmentPool.misses = 0;
    
    // Initialize coord array pool with pre-allocated arrays
    coordArrayPool.coordArrays.clear();
    coordArrayPool.coordArrays.reserve(coordArrayPool.maxSize);
    
    // Pre-allocate coordinate arrays
    ESP_LOGI(TAG, "Pre-allocating %zu coordinate arrays...", coordArrayPool.maxSize);
    for (size_t i = 0; i < coordArrayPool.maxSize; i++) {
        int* array = allocBuffer<int>(256); // Pre-allocate arrays of 256 ints
        if (array) {
            std::vector<int*> arrays;
            arrays.push_back(array);
            coordArrayPool.coordArrays.push_back(arrays);
        } else {
            ESP_LOGW(TAG, "Failed to allocate coordinate array %zu", i);
        }
    }
    ESP_LOGI(TAG, "Pre-allocated %zu coordinate arrays", coordArrayPool.coordArrays.size());
    
    coordArrayPool.hits = 0;
    coordArrayPool.misses = 0;
    
    ESP_LOGI(TAG, "Advanced memory pools initialized");
    ESP_LOGI(TAG, "Point pool: %zu, Command pool: %zu, Coords pool: %zu, Feature pool: %zu, LineBatch pool: %zu, CoordArray pool: %zu",
             pointPool.maxSize, commandPool.maxSize, coordsPool.maxSize, featurePool.maxSize, lineSegmentPool.maxSize, coordArrayPool.maxSize);
}

/**
 * @brief Detect advanced pool capabilities and set pool sizes
 *
 * @details Automatically detects available memory and sets appropriate pool sizes.
 */
void Maps::detectAdvancedPoolCapabilities()
{
#ifdef BOARD_HAS_PSRAM
    size_t psramFree = ESP.getFreePsram();
    
    // Calculate pool sizes based on available PSRAM
    pointPool.maxSize = std::min(static_cast<size_t>(2000), psramFree / (1024 * 4)); // 4KB per point pool
    commandPool.maxSize = std::min(static_cast<size_t>(1000), psramFree / (1024 * 8)); // 8KB per command pool
    coordsPool.maxSize = std::min(static_cast<size_t>(500), psramFree / (1024 * 16)); // 16KB per coords pool
    featurePool.maxSize = std::min(static_cast<size_t>(200), psramFree / (1024 * 32)); // 32KB per feature pool
    lineSegmentPool.maxSize = std::min(static_cast<size_t>(100), psramFree / (1024 * 64)); // 64KB per line batch pool
    coordArrayPool.maxSize = std::min(static_cast<size_t>(50), psramFree / (1024 * 128)); // 128KB per coord array pool
#else
    size_t ramFree = ESP.getFreeHeap();
    
    // Smaller pools for ESP32 without PSRAM
    pointPool.maxSize = std::min(static_cast<size_t>(500), ramFree / (1024 * 8));
    commandPool.maxSize = std::min(static_cast<size_t>(250), ramFree / (1024 * 16));
    coordsPool.maxSize = std::min(static_cast<size_t>(100), ramFree / (1024 * 32));
    featurePool.maxSize = std::min(static_cast<size_t>(50), ramFree / (1024 * 64));
    lineSegmentPool.maxSize = std::min(static_cast<size_t>(25), ramFree / (1024 * 128));
    coordArrayPool.maxSize = std::min(static_cast<size_t>(10), ramFree / (1024 * 256));
#endif
    
    ESP_LOGI(TAG, "Advanced pool sizes: Points=%zu, Commands=%zu, Coords=%zu, Features=%zu, LineBatches=%zu, CoordArrays=%zu",
             pointPool.maxSize, commandPool.maxSize, coordsPool.maxSize, featurePool.maxSize, lineSegmentPool.maxSize, coordArrayPool.maxSize);
}

/**
 * @brief Get point from pool
 *
 * @details Gets a point from the pool or creates a new one.
 *
 * @return Point pair (x, y).
 */
std::pair<int, int> Maps::getPoint()
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (!pointPool.points.empty()) {
            auto point = pointPool.points.back();
            pointPool.points.pop_back();
            pointPool.hits++;
            xSemaphoreGive(advancedPoolMutex);
            return point;
        }
        pointPool.misses++;
        xSemaphoreGive(advancedPoolMutex);
    }
    return std::make_pair(0, 0);
}

/**
 * @brief Return point to pool
 *
 * @details Returns a point to the pool for reuse.
 *
 * @param point The point to return.
 */
void Maps::returnPoint(const std::pair<int, int>& point)
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (pointPool.points.size() < pointPool.maxSize) {
            pointPool.points.push_back(point);
        }
        xSemaphoreGive(advancedPoolMutex);
    }
}

/**
 * @brief Get command from pool
 *
 * @details Gets a command from the pool or creates a new one.
 *
 * @return Command pair (type, color).
 */
std::pair<uint8_t, uint16_t> Maps::getCommand()
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (!commandPool.commands.empty()) {
            auto command = commandPool.commands.back();
            commandPool.commands.pop_back();
            commandPool.hits++;
            xSemaphoreGive(advancedPoolMutex);
            return command;
        }
        commandPool.misses++;
        xSemaphoreGive(advancedPoolMutex);
    }
    return std::make_pair(0, 0);
}

/**
 * @brief Return command to pool
 *
 * @details Returns a command to the pool for reuse.
 *
 * @param command The command to return.
 */
void Maps::returnCommand(const std::pair<uint8_t, uint16_t>& command)
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (commandPool.commands.size() < commandPool.maxSize) {
            commandPool.commands.push_back(command);
        }
        xSemaphoreGive(advancedPoolMutex);
    }
}

/**
 * @brief Get coordinates from pool
 *
 * @details Gets coordinates from the pool or creates a new vector.
 *
 * @return Coordinates vector.
 */
std::vector<std::pair<int, int>> Maps::getCoords()
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (!coordsPool.coords.empty()) {
            auto coords = coordsPool.coords.back();
            coordsPool.coords.pop_back();
            coords.clear(); // Clear for reuse
            coordsPool.hits++;
            xSemaphoreGive(advancedPoolMutex);
            return coords;
        }
        coordsPool.misses++;
        xSemaphoreGive(advancedPoolMutex);
    }
    return std::vector<std::pair<int, int>>();
}

/**
 * @brief Return coordinates to pool
 *
 * @details Returns coordinates to the pool for reuse.
 *
 * @param coords The coordinates to return.
 */
void Maps::returnCoords(const std::vector<std::pair<int, int>>& coords)
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (coordsPool.coords.size() < coordsPool.maxSize) {
            coordsPool.coords.push_back(coords);
        }
        xSemaphoreGive(advancedPoolMutex);
    }
}

/**
 * @brief Get feature from pool
 *
 * @details Gets a feature from the pool or creates a new map.
 *
 * @return Feature map.
 */
std::map<std::string, std::string> Maps::getFeature()
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (!featurePool.features.empty()) {
            auto feature = featurePool.features.back();
            featurePool.features.pop_back();
            feature.clear(); // Clear for reuse
            featurePool.hits++;
            xSemaphoreGive(advancedPoolMutex);
            return feature;
        }
        featurePool.misses++;
        xSemaphoreGive(advancedPoolMutex);
    }
    return std::map<std::string, std::string>();
}

/**
 * @brief Return feature to pool
 *
 * @details Returns a feature to the pool for reuse.
 *
 * @param feature The feature to return.
 */
void Maps::returnFeature(const std::map<std::string, std::string>& feature)
{
    if (advancedPoolMutex && xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        if (featurePool.features.size() < featurePool.maxSize) {
            featurePool.features.push_back(feature);
        }
        xSemaphoreGive(advancedPoolMutex);
    }
}

/**
 * @brief Clear all advanced pools
 *
 * @details Clears all advanced pool entries.
 */
void Maps::clearAdvancedPools()
{
    if (!advancedPoolMutex) return;
    
    if (xSemaphoreTake(advancedPoolMutex, portMAX_DELAY) == pdTRUE) {
        pointPool.points.clear();
        commandPool.commands.clear();
        coordsPool.coords.clear();
        featurePool.features.clear();
        lineSegmentPool.lineBatches.clear();
        coordArrayPool.coordArrays.clear();
        
        pointPool.hits = pointPool.misses = 0;
        commandPool.hits = commandPool.misses = 0;
        coordsPool.hits = coordsPool.misses = 0;
        featurePool.hits = featurePool.misses = 0;
        lineSegmentPool.hits = lineSegmentPool.misses = 0;
        coordArrayPool.hits = coordArrayPool.misses = 0;
        
        xSemaphoreGive(advancedPoolMutex);
    }
}

/**
 * @brief Print advanced pool statistics
 *
 * @details Prints useful advanced pool statistics for debugging.
 */
void Maps::printAdvancedPoolStats()
{
    ESP_LOGI(TAG, "=== Advanced Memory Pool Statistics ===");
    ESP_LOGI(TAG, "Point Pool: %zu/%zu used, %u hits, %u misses", 
             pointPool.points.size(), pointPool.maxSize, pointPool.hits, pointPool.misses);
    ESP_LOGI(TAG, "Command Pool: %zu/%zu used, %u hits, %u misses", 
             commandPool.commands.size(), commandPool.maxSize, commandPool.hits, commandPool.misses);
    ESP_LOGI(TAG, "Coords Pool: %zu/%zu used, %u hits, %u misses", 
             coordsPool.coords.size(), coordsPool.maxSize, coordsPool.hits, coordsPool.misses);
    ESP_LOGI(TAG, "Feature Pool: %zu/%zu used, %u hits, %u misses", 
             featurePool.features.size(), featurePool.maxSize, featurePool.hits, featurePool.misses);
    ESP_LOGI(TAG, "LineSegment Pool: %zu/%zu used, %u hits, %u misses", 
             lineSegmentPool.lineBatches.size(), lineSegmentPool.maxSize, lineSegmentPool.hits, lineSegmentPool.misses);
    ESP_LOGI(TAG, "CoordArray Pool: %zu/%zu used, %u hits, %u misses", 
             coordArrayPool.coordArrays.size(), coordArrayPool.maxSize, coordArrayPool.hits, coordArrayPool.misses);
}

void Maps::initializeAdvancedPools()
{
    initAdvancedPools();
}

void Maps::printAdvancedMemoryPoolStats()
{
    printAdvancedPoolStats();
}

/**
 * @brief Initialize memory monitoring system
 *
 * @details Initializes the memory monitoring system with initial values.
 */
void Maps::initMemoryMonitoring()
{
    totalMemoryAllocations = 0;
    totalMemoryDeallocations = 0;
    peakMemoryUsage = 0;
    currentMemoryUsage = 0;
    poolEfficiencyScore = 0;
    lastStatsUpdate = millis();
    
    ESP_LOGI(TAG, "Memory monitoring system initialized");
}

/**
 * @brief Update memory statistics
 *
 * @details Updates current memory usage and calculates efficiency metrics.
 */
void Maps::updateMemoryStats()
{
    uint32_t currentTime = millis();
    
    // Update every tile (force update)
    currentMemoryUsage = ESP.getFreeHeap();
    
    // Update peak memory usage
    if (currentMemoryUsage > peakMemoryUsage) {
        peakMemoryUsage = currentMemoryUsage;
    }
    
    // Calculate pool efficiency
    calculatePoolEfficiency();
    
    lastStatsUpdate = currentTime;
}

/**
 * @brief Calculate pool efficiency score
 *
 * @details Calculates efficiency score based on hit/miss ratios.
 */
void Maps::calculatePoolEfficiency()
{
    uint32_t totalHits = pointPool.hits + commandPool.hits + coordsPool.hits + 
                        featurePool.hits + lineSegmentPool.hits + coordArrayPool.hits;
    uint32_t totalMisses = pointPool.misses + commandPool.misses + coordsPool.misses + 
                          featurePool.misses + lineSegmentPool.misses + coordArrayPool.misses;
    
    if (totalHits + totalMisses > 0) {
        poolEfficiencyScore = (totalHits * 100) / (totalHits + totalMisses);
    } else {
        poolEfficiencyScore = 0;
    }
}

/**
 * @brief Print detailed memory statistics
 *
 * @details Prints comprehensive memory usage and efficiency statistics.
 */
void Maps::printMemoryStats()
{
    ESP_LOGI(TAG, "=== Memory Statistics ===");
    ESP_LOGI(TAG, "Total Allocations: %u", totalMemoryAllocations);
    ESP_LOGI(TAG, "Total Deallocations: %u", totalMemoryDeallocations);
    ESP_LOGI(TAG, "Current Memory Usage: %u bytes", currentMemoryUsage);
    ESP_LOGI(TAG, "Peak Memory Usage: %u bytes", peakMemoryUsage);
    ESP_LOGI(TAG, "Pool Efficiency Score: %u%%", poolEfficiencyScore);
    
#ifdef BOARD_HAS_PSRAM
    ESP_LOGI(TAG, "PSRAM Free: %u bytes", ESP.getFreePsram());
#endif
    ESP_LOGI(TAG, "Heap Free: %u bytes", ESP.getFreeHeap());
    ESP_LOGI(TAG, "Heap Min Free: %u bytes", ESP.getMinFreeHeap());
}

/**
 * @brief Reset memory statistics
 *
 * @details Resets all memory statistics to initial values.
 */
void Maps::resetMemoryStats()
{
    totalMemoryAllocations = 0;
    totalMemoryDeallocations = 0;
    peakMemoryUsage = 0;
    currentMemoryUsage = 0;
    poolEfficiencyScore = 0;
    lastStatsUpdate = millis();
    
    ESP_LOGI(TAG, "Memory statistics reset");
}

/**
 * @brief Check if memory pressure is high
 *
 * @details Checks if system is under high memory pressure.
 *
 * @return true if memory pressure is high, false otherwise.
 */
bool Maps::isMemoryPressureHigh()
{
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    
    // High pressure if less than 50KB free heap or min free is very low
    return (freeHeap < 51200) || (minFreeHeap < 10240);
}

void Maps::initializeMemoryMonitoring()
{
    initMemoryMonitoring();
}

void Maps::printMemoryMonitoringStats()
{
    printMemoryStats();
}
void Maps::initMemoryPool()
{
    detectMemoryPoolCapabilities();
    
    if (memoryPoolMutex == nullptr) {
        memoryPoolMutex = xSemaphoreCreateMutex();
        if (memoryPoolMutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create memory pool mutex");
            return;
        }
    }
    
    memoryPool.clear();
    memoryPool.reserve(maxPoolEntries);
    poolAllocationCount = 0;
    poolHitCount = 0;
    poolMissCount = 0;
    
    ESP_LOGI(TAG, "Memory pool initialized with %zu entries capacity", maxPoolEntries);
}

/**
 * @brief Detect memory pool capabilities and set pool size
 *
 * @details Automatically detects available memory and sets the appropriate pool size.
 */
void Maps::detectMemoryPoolCapabilities()
{
    maxPoolEntries = 0; // Default: no pool
    
#ifdef BOARD_HAS_PSRAM
    size_t psramFree = ESP.getFreePsram();
    size_t ramFree = ESP.getFreeHeap();
    
    ESP_LOGI(TAG, "PSRAM free: %zu bytes (%.2f MB)", psramFree, psramFree / (1024.0 * 1024.0));
    ESP_LOGI(TAG, "RAM free: %zu bytes (%.2f KB)", ramFree, ramFree / 1024.0);
    
    // Calculate pool size based on available memory
    // Reserve 20% of free PSRAM for pool (small buffers for line segments, etc.)
    size_t poolMemory = psramFree * 0.2;
    size_t avgBufferSize = 1024; // Average buffer size for line segments, polygons, etc.
    
    maxPoolEntries = poolMemory / avgBufferSize;
    
    // Apply reasonable limits
    if (maxPoolEntries > 16) maxPoolEntries = 16; // Max 16 entries
    if (maxPoolEntries < 4) maxPoolEntries = 4;   // Min 4 entries
    
    ESP_LOGI(TAG, "Memory pool capacity: %zu entries (%.2f KB reserved)", 
             maxPoolEntries, poolMemory / 1024.0);
#else
    // ESP32 without PSRAM: small pool in RAM
    size_t ramFree = ESP.getFreeHeap();
    size_t poolMemory = ramFree * 0.1; // Only 10% of RAM for pool
    size_t avgBufferSize = 512; // Smaller buffers for limited RAM
    
    maxPoolEntries = poolMemory / avgBufferSize;
    if (maxPoolEntries > 8) maxPoolEntries = 8;  // Max 8 entries
    if (maxPoolEntries < 2) maxPoolEntries = 0;  // Disable if too small
    
    ESP_LOGI(TAG, "Memory pool capacity: %zu entries (%.2f KB reserved)", 
             maxPoolEntries, poolMemory / 1024.0);
#endif
}

/**
 * @brief Allocate memory from pool
 *
 * @details Attempts to allocate memory from the pool. If no suitable entry is found, returns nullptr.
 *
 * @param size The size of memory to allocate.
 * @return Pointer to allocated memory, or nullptr if not available.
 */
void* Maps::poolAllocate(size_t size)
{
    if (maxPoolEntries == 0 || !memoryPoolMutex) return nullptr;
    
    if (xSemaphoreTake(memoryPoolMutex, portMAX_DELAY) == pdTRUE) {
        // Look for available entry of suitable size
        for (auto& entry : memoryPool) {
            if (!entry.isInUse && entry.size >= size) {
                entry.isInUse = true;
                entry.allocationCount++;
                poolAllocationCount++;
                xSemaphoreGive(memoryPoolMutex);
                return entry.ptr;
            }
        }
        
        // No suitable entry found, try to create new one
        if (memoryPool.size() < maxPoolEntries) {
            MemoryPoolEntry newEntry;
            
#ifdef BOARD_HAS_PSRAM
            newEntry.ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
#else
            newEntry.ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);
#endif
            
            if (newEntry.ptr) {
                newEntry.size = size;
                newEntry.isInUse = true;
                newEntry.allocationCount = 1;
                memoryPool.push_back(newEntry);
                poolAllocationCount++;
                xSemaphoreGive(memoryPoolMutex);
                return newEntry.ptr;
            }
        }
        
        xSemaphoreGive(memoryPoolMutex);
    }
    
    return nullptr; // Pool allocation failed
}

/**
 * @brief Return memory to pool
 *
 * @details Returns memory to the pool by marking the entry as available.
 *
 * @param ptr Pointer to memory to return.
 */
void Maps::poolDeallocate(void* ptr)
{
    if (!ptr || maxPoolEntries == 0 || !memoryPoolMutex) return;
    
    if (xSemaphoreTake(memoryPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : memoryPool) {
            if (entry.ptr == ptr && entry.isInUse) {
                entry.isInUse = false;
                xSemaphoreGive(memoryPoolMutex);
                return;
            }
        }
        xSemaphoreGive(memoryPoolMutex);
    }
}

/**
 * @brief Clear all pool entries
 *
 * @details Frees all pool entries and clears the pool.
 */
void Maps::clearMemoryPool()
{
    if (!memoryPoolMutex) return;
    
    if (xSemaphoreTake(memoryPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : memoryPool) {
            if (entry.ptr) {
                heap_caps_free(entry.ptr);
            }
        }
        memoryPool.clear();
        poolAllocationCount = 0;
        poolHitCount = 0;
        poolMissCount = 0;
        xSemaphoreGive(memoryPoolMutex);
    }
}

/**
 * @brief Get current pool memory usage
 *
 * @details Calculates the current memory usage of the pool.
 *
 * @return Memory usage in bytes.
 */
size_t Maps::getPoolMemoryUsage()
{
    size_t memoryUsage = 0;
    if (!memoryPoolMutex) return 0;
    
    if (xSemaphoreTake(memoryPoolMutex, 0) == pdTRUE) {
        for (const auto& entry : memoryPool) {
            memoryUsage += entry.size;
        }
        xSemaphoreGive(memoryPoolMutex);
    }
    return memoryUsage;
}

/**
 * @brief Print pool statistics for debugging
 *
 * @details Prints useful pool statistics for debugging and monitoring.
 */
void Maps::printPoolStats()
{
    ESP_LOGI(TAG, "=== Memory Pool Statistics ===");
    ESP_LOGI(TAG, "Max pool entries: %zu", maxPoolEntries);
    ESP_LOGI(TAG, "Current pool entries: %zu", memoryPool.size());
    ESP_LOGI(TAG, "Pool memory usage: %zu bytes", getPoolMemoryUsage());
    ESP_LOGI(TAG, "Total allocations: %u", poolAllocationCount);
    ESP_LOGI(TAG, "Pool hits: %u", poolHitCount);
    ESP_LOGI(TAG, "Pool misses: %u", poolMissCount);
    
    if (poolAllocationCount > 0) {
        float hitRate = (float)poolHitCount / poolAllocationCount * 100.0f;
        ESP_LOGI(TAG, "Hit rate: %.1f%%", hitRate);
    }
    
    for (size_t i = 0; i < memoryPool.size(); ++i) {
        const auto& entry = memoryPool[i];
        ESP_LOGI(TAG, "Pool[%zu]: %zu bytes, inUse: %s, allocations: %u", 
                 i, entry.size, entry.isInUse ? "yes" : "no", entry.allocationCount);
    }
    ESP_LOGI(TAG, "=============================");
}

/**
 * @brief Initialize memory pool system (public method)
 *
 * @details Public method to initialize the memory pool system.
 */
void Maps::initializeMemoryPool()
{
    initMemoryPool();
}

/**
 * @brief Print memory pool statistics (public method)
 *
 * @details Public method to print memory pool statistics.
 */
void Maps::printMemoryPoolStats()
{
    printPoolStats();
}

/**
 * @brief Initialize polygon optimization system
 *
 * @details Initializes the polygon optimization system with default settings.
 */
void Maps::initPolygonOptimizations()
{
    // Step-by-step polygon optimizations - enable culling first
    polygonCullingEnabled = true;   // Enable culling - safe optimization
    optimizedScanlineEnabled = false; // Keep scanline disabled for now
    polygonRenderCount = 0;
    polygonCulledCount = 0;
    polygonOptimizedCount = 0;
    
    ESP_LOGI(TAG, "Polygon optimizations initialized - Culling: %s, Optimized scanline: %s", 
             polygonCullingEnabled ? "enabled" : "disabled",
             optimizedScanlineEnabled ? "enabled" : "disabled");
}

/**
 * @brief Initialize layer rendering system
 *
 * @details Initializes the layer rendering system for proper map layer ordering.
 */
void Maps::initLayerRendering()
{
    layerRenderingEnabled = false;  // Disable layer rendering - tiles already ordered by layer
    layerRenderCount = 0;
    
    // Clear all layer commands
    for (int i = 0; i < LAYER_COUNT; i++) {
        layerCommands[i].clear();
    }
    
    ESP_LOGI(TAG, "Layer rendering system initialized (DISABLED - tiles pre-ordered)", LAYER_COUNT);
}

/**
 * @brief Determine which layer a command belongs to
 *
 * @details Analyzes command type and color to determine the appropriate rendering layer.
 *
 * @param cmdType The command type (DrawCommand enum)
 * @param color The RGB332 color value
 * @return RenderLayer The appropriate layer for this command
 */
Maps::RenderLayer Maps::determineCommandLayer(uint8_t cmdType, uint8_t color)
{
    // Extract RGB components for analysis
    uint8_t r = (color >> 5) & 0x7;
    uint8_t g = (color >> 2) & 0x7;
    uint8_t b = color & 0x3;
    
    // Determine layer based on command type and color
    switch (cmdType) {
        case SET_LAYER:
            // SET_LAYER command doesn't determine a layer, it sets one
            // This should not be called for SET_LAYER commands
            return LAYER_TERRAIN; // Default fallback
            
        case DRAW_STROKE_POLYGON:
        case DRAW_STROKE_POLYGONS:
        case RECTANGLE:
        case CIRCLE:
        case SIMPLE_RECTANGLE:
        case SIMPLE_CIRCLE:
        case SIMPLE_TRIANGLE:
        case OPTIMIZED_POLYGON:
        case HOLLOW_POLYGON:
        case OPTIMIZED_TRIANGLE:
        case OPTIMIZED_RECTANGLE:
        case OPTIMIZED_CIRCLE:
        case BLOCK_PATTERN:
            // Filled shapes - determine by color
            if (b > g && b > r) {
                // Blue dominant - water
                return LAYER_WATER;
            } else if (r > 4 || g > 4) {
                // Red/brown dominant - buildings
                return LAYER_BUILDINGS;
            } else {
                // Green/neutral - terrain
                return LAYER_TERRAIN;
            }
            
        case DRAW_LINE:
        case DRAW_POLYLINE:
        case DRAW_HORIZONTAL_LINE:
        case DRAW_VERTICAL_LINE:
        case STRAIGHT_LINE:
        case DASHED_LINE:
        case DOTTED_LINE:
        case GRID_PATTERN:
        case COMPRESSED_POLYLINE:
        case PREDICTED_LINE:
        case RELATIVE_MOVE:
        case HIGHWAY_SEGMENT:
            // Lines - determine by color
            if (r > 4 && g < 3 && b < 2) {
                // Red dominant - roads
                return LAYER_ROADS;
            } else if (r > 3 || g > 3 || b > 2) {
                // Colored lines - outlines
                return LAYER_OUTLINES;
            } else {
                // Dark lines - roads
                return LAYER_ROADS;
            }
            
        default:
            // Default to terrain for unknown commands
            return LAYER_TERRAIN;
    }
}

/**
 * @brief Clear all layer commands
 *
 * @details Clears all commands from all layers, freeing associated memory.
 */
void Maps::clearLayerCommands()
{
    for (int i = 0; i < LAYER_COUNT; i++) {
        // Free command data
        for (auto& cmd : layerCommands[i]) {
            if (cmd.data) {
                poolDeallocate(cmd.data);
            }
        }
        layerCommands[i].clear();
    }
    
    ESP_LOGI(TAG, "Cleared all layer commands");
}

/**
 * @brief Render a specific layer
 *
 * @details Renders all commands in a specific layer with proper ordering.
 *
 * @param map The TFT_eSprite object where the layer will be rendered.
 * @param commands The vector of commands to render.
 * @param xOffset The x-offset to apply when rendering.
 * @param yOffset The y-offset to apply when rendering.
 */
void Maps::renderLayer(TFT_eSprite& map, const std::vector<RenderCommand>& commands, int xOffset, int yOffset)
{
    for (const auto& cmd : commands) {
        // Set current color
        uint16_t currentDrawColor = cmd.colorRGB565;
        
        // Render based on command type
        switch (cmd.type) {
            case SET_LAYER:
                // SET_LAYER command - extract layer number and set current layer
                {
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int)) {
                        int layerNumber = data[0];
                        // Layer is already determined by the layer system, so we just continue
                        // This command is mainly for compatibility with tile_viewer.py
                    }
                }
                break;
                
            case DRAW_STROKE_POLYGON:
            case DRAW_STROKE_POLYGONS:
            case OPTIMIZED_POLYGON:
            case HOLLOW_POLYGON:
                {
                    // Extract polygon data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int)) {
                        int numPoints = data[0];
                        if (numPoints >= 3 && cmd.dataSize >= sizeof(int) * (1 + numPoints * 2)) {
                            int* px = data + 1;
                            int* py = px + numPoints;
                            
                            // Fill polygon
                            if (fillPolygons && cmd.type != HOLLOW_POLYGON) {
                                fillPolygonOptimized(map, px, py, numPoints, currentDrawColor, xOffset, yOffset);
                                
                                // Draw border
                                const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                                drawPolygonBorder(map, px, py, numPoints, borderColor, currentDrawColor, xOffset, yOffset);
                            } else {
                                // Draw only outline for HOLLOW_POLYGON or when fillPolygons is false
                                drawPolygonBorder(map, px, py, numPoints, currentDrawColor, currentDrawColor, xOffset, yOffset);
                            }
                        }
                    }
                }
                break;
                
            case RECTANGLE:
                {
                    // Extract rectangle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 4) {
                        int x = data[0] + xOffset;
                        int y = data[1] + yOffset;
                        int w = data[2];
                        int h = data[3];
                        
                        if (fillPolygons) {
                            map.fillRect(x, y, w, h, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            map.drawRect(x, y, w, h, borderColor);
                        }
                    }
                }
                break;
                
            case SIMPLE_RECTANGLE:
                {
                    // Extract rectangle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 4) {
                        int x = data[0] + xOffset;
                        int y = data[1] + yOffset;
                        int w = data[2];
                        int h = data[3];
                        
                        if (fillPolygons) {
                            map.fillRect(x, y, w, h, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            map.drawRect(x, y, w, h, borderColor);
                        } else {
                            map.drawRect(x, y, w, h, currentDrawColor);
                        }
                    }
                }
                break;
                
            case OPTIMIZED_RECTANGLE:
                {
                    // Extract rectangle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 4) {
                        int x = data[0] + xOffset;
                        int y = data[1] + yOffset;
                        int w = data[2];
                        int h = data[3];
                        
                        if (fillPolygons) {
                            map.fillRect(x, y, w, h, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            map.drawRect(x, y, w, h, borderColor);
                        } else {
                            map.drawRect(x, y, w, h, currentDrawColor);
                        }
                    }
                }
                break;
                
            case CIRCLE:
                {
                    // Extract circle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 3) {
                        int cx = data[0] + xOffset;
                        int cy = data[1] + yOffset;
                        int radius = data[2];
                        
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            drawFilledCircleWithBorder(map, cx, cy, radius, currentDrawColor, borderColor, fillPolygons);
                        }
                    }
                }
                break;
                
            case OPTIMIZED_TRIANGLE:
                {
                    // Extract triangle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 6) {
                        int x1 = data[0] + xOffset;
                        int y1 = data[1] + yOffset;
                        int x2 = data[2] + xOffset;
                        int y2 = data[3] + yOffset;
                        int x3 = data[4] + xOffset;
                        int y3 = data[5] + yOffset;
                        
                        if (fillPolygons) {
                            // Fill triangle using optimized algorithm
                            fillTriangleOptimized(map, x1, y1, x2, y2, x3, y3, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            map.drawLine(x1, y1, x2, y2, borderColor);
                            map.drawLine(x2, y2, x3, y3, borderColor);
                            map.drawLine(x3, y3, x1, y1, borderColor);
                        } else {
                            // Draw only outline
                            map.drawLine(x1, y1, x2, y2, currentDrawColor);
                            map.drawLine(x2, y2, x3, y3, currentDrawColor);
                            map.drawLine(x3, y3, x1, y1, currentDrawColor);
                        }
                    }
                }
                break;
                
            case SIMPLE_CIRCLE:
                {
                    // Extract circle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 3) {
                        int cx = data[0] + xOffset;
                        int cy = data[1] + yOffset;
                        int radius = data[2];
                        
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            drawFilledCircleWithBorder(map, cx, cy, radius, currentDrawColor, borderColor, fillPolygons);
                        } else {
                            map.drawCircle(cx, cy, radius, currentDrawColor);
                        }
                    }
                }
                break;
                
            case OPTIMIZED_CIRCLE:
                {
                    // Extract circle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 3) {
                        int cx = data[0] + xOffset;
                        int cy = data[1] + yOffset;
                        int radius = data[2];
                        
                        if (fillPolygons) {
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            drawFilledCircleWithBorder(map, cx, cy, radius, currentDrawColor, borderColor, fillPolygons);
                        } else {
                            map.drawCircle(cx, cy, radius, currentDrawColor);
                        }
                    }
                }
                break;
                
            case SIMPLE_TRIANGLE:
                {
                    // Extract triangle data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 6) {
                        int x1 = data[0] + xOffset;
                        int y1 = data[1] + yOffset;
                        int x2 = data[2] + xOffset;
                        int y2 = data[3] + yOffset;
                        int x3 = data[4] + xOffset;
                        int y3 = data[5] + yOffset;
                        
                        if (fillPolygons) {
                            // Fill triangle using optimized algorithm
                            fillTriangleOptimized(map, x1, y1, x2, y2, x3, y3, currentDrawColor);
                            
                            // Draw border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(cmd.color));
                            map.drawLine(x1, y1, x2, y2, borderColor);
                            map.drawLine(x2, y2, x3, y3, borderColor);
                            map.drawLine(x3, y3, x1, y1, borderColor);
                        } else {
                            // Draw only outline
                            map.drawLine(x1, y1, x2, y2, currentDrawColor);
                            map.drawLine(x2, y2, x3, y3, currentDrawColor);
                            map.drawLine(x3, y3, x1, y1, currentDrawColor);
                        }
                    }
                }
                break;
                
            case DRAW_LINE:
            case DRAW_POLYLINE:
            case DRAW_HORIZONTAL_LINE:
            case DRAW_VERTICAL_LINE:
            case STRAIGHT_LINE:
                {
                    // Extract line data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 4) {
                        int x1 = data[0] + xOffset;
                        int y1 = data[1] + yOffset;
                        int x2 = data[2] + xOffset;
                        int y2 = data[3] + yOffset;
                        
                        map.drawLine(x1, y1, x2, y2, currentDrawColor);
                    }
                }
                break;
                
            case COMPRESSED_POLYLINE:
                {
                    // Extract compressed polyline data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 2) {
                        int numPoints = data[0];
                        int* points = &data[1];
                        
                        // Draw connected lines
                        for (int i = 0; i < numPoints - 1; i++) {
                            int x1 = points[i * 2] + xOffset;
                            int y1 = points[i * 2 + 1] + yOffset;
                            int x2 = points[(i + 1) * 2] + xOffset;
                            int y2 = points[(i + 1) * 2 + 1] + yOffset;
                            
                            map.drawLine(x1, y1, x2, y2, currentDrawColor);
                        }
                    }
                }
                break;
                
            case PREDICTED_LINE:
                {
                    // Extract predicted line data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 4) {
                        int startX = data[0] + xOffset;
                        int startY = data[1] + yOffset;
                        int patternType = data[2];
                        int length = data[3];
                        
                        // Draw predicted line based on pattern
                        drawPredictedLine(map, startX, startY, patternType, length, currentDrawColor);
                    }
                }
                break;
                
            case RELATIVE_MOVE:
                {
                    // RELATIVE_MOVE doesn't draw anything, it just updates the current position
                    // This is handled in the command processing loop, not in renderLayer
                    // Skip rendering for this command type
                }
                break;
                
            case HIGHWAY_SEGMENT:
                {
                    // Extract highway segment data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 5) {
                        int startX = data[0] + xOffset;
                        int startY = data[1] + yOffset;
                        int endX = data[2] + xOffset;
                        int endY = data[3] + yOffset;
                        int width = data[4];
                        
                        // Draw highway segment with width
                        drawHighwaySegment(map, startX, startY, endX, endY, width, currentDrawColor);
                    }
                }
                break;
                
            case BLOCK_PATTERN:
                {
                    // Extract block pattern data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 5) {
                        int x = data[0] + xOffset;
                        int y = data[1] + yOffset;
                        int blockSize = data[2];
                        int spacing = data[3];
                        int count = data[4];
                        
                        // Draw block pattern
                        drawBlockPattern(map, x, y, blockSize, spacing, count, currentDrawColor);
                    }
                }
                break;
                
            case DASHED_LINE:
                {
                    // Extract dashed line data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 6) {
                        int x1 = data[0] + xOffset;
                        int y1 = data[1] + yOffset;
                        int x2 = data[2] + xOffset;
                        int y2 = data[3] + yOffset;
                        int dashLength = data[4];
                        int gapLength = data[5];
                        
                        // Draw dashed line using Bresenham-like algorithm
                        drawDashedLine(map, x1, y1, x2, y2, dashLength, gapLength, currentDrawColor);
                    }
                }
                break;
                
            case DOTTED_LINE:
                {
                    // Extract dotted line data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 5) {
                        int x1 = data[0] + xOffset;
                        int y1 = data[1] + yOffset;
                        int x2 = data[2] + xOffset;
                        int y2 = data[3] + yOffset;
                        int dotSpacing = data[4];
                        
                        // Draw dotted line
                        drawDottedLine(map, x1, y1, x2, y2, dotSpacing, currentDrawColor);
                    }
                }
                break;
                
            case GRID_PATTERN:
                {
                    // Extract grid pattern data
                    int* data = static_cast<int*>(cmd.data);
                    if (data && cmd.dataSize >= sizeof(int) * 6) {
                        int x = data[0] + xOffset;
                        int y = data[1] + yOffset;
                        int width = data[2];
                        int spacing = data[3];
                        int count = data[4];
                        int direction = data[5];
                        
                        // Draw grid pattern
                        drawGridPattern(map, x, y, width, spacing, count, direction, currentDrawColor);
                    }
                }
                break;
                
            default:
                // Skip unknown commands
                break;
        }
    }
}

/**
 * @brief Render tile with proper layer ordering
 *
 * @details Reads tile data, organizes commands by layers, and renders them in the correct order:
 *         Terrain → Water → Buildings → Outlines → Roads
 *
 * @param path The file path to the binary map tile.
 * @param xOffset The x-offset to apply when rendering the tile on the sprite.
 * @param yOffset The y-offset to apply when rendering the tile on the sprite.
 * @param map The TFT_eSprite object where the tile will be rendered.
 * @return true if the tile was rendered successfully, false otherwise.
 */
bool Maps::renderTileLayered(const char* path, const int16_t xOffset, const int16_t yOffset, TFT_eSprite& map)
{
    static bool isPaletteLoaded = false;
    if (!isPaletteLoaded) {
        isPaletteLoaded = Maps::loadPalette("/sdcard/VECTMAP/palette.bin");
        if (isPaletteLoaded) {
            Serial.printf("Palette loaded successfully: %d colors\n", PALETTE_SIZE);
        } else {
            Serial.println("WARNING: Failed to load palette from /sdcard/VECTMAP/palette.bin");
        }
    }

    if (!path || path[0] == '\0')
        return false;

    // Try to get tile from cache first
    if (getCachedTile(path, map, xOffset, yOffset)) {
        return true; // Tile found in cache
    }

    FILE* file = fopen(path, "rb");
    if (!file)
       return false;
    
    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = allocBuffer<uint8_t>(fileSize);
    if (!data)
    {
        fclose(file);
        return false;
    }

    const size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) 
    {
        poolDeallocate(data);
        return false;
    }

    size_t offset = 0;
    const size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = RGB332ToRGB565(current_color);

    const uint32_t num_cmds = readVarint(data, offset, dataSize);
    if (num_cmds == 0)
    {
        poolDeallocate(data);
        return false;
    }

    // Clear all layer commands efficiently
    for (int i = 0; i < LAYER_COUNT; i++) {
        // Free command data before clearing
        for (auto& cmd : layerCommands[i]) {
            if (cmd.data) {
                poolDeallocate(cmd.data);
            }
        }
        layerCommands[i].clear();
    }
    
    int executed = 0;
    unsigned long renderStart = millis();

    // First pass: organize commands by layers with memory optimization
    int currentLayer = LAYER_TERRAIN;  // Default layer
    
    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) 
    {
        if (offset >= dataSize) 
            break;
        const size_t cmdStartOffset = offset;
        const uint32_t cmdType = readVarint(data, offset, dataSize);

        switch (cmdType) 
        {
            case SET_LAYER:
                {
                    const int32_t layerNumber = readVarint(data, offset, dataSize);
                    // Update current layer for subsequent commands
                    currentLayer = static_cast<RenderLayer>(layerNumber);
                    if (currentLayer >= LAYER_COUNT) {
                        currentLayer = LAYER_TERRAIN;  // Fallback to terrain
                    }
                    continue;  // Skip to next command after SET_LAYER
                }
                break;
                
            case SET_COLOR:
                if (offset < dataSize) 
                {
                    current_color = data[offset++];
                    currentDrawColor = RGB332ToRGB565(current_color);
                }
                break;
                
            case SET_COLOR_INDEX:
                {
                    const uint32_t color_index = readVarint(data, offset, dataSize);
                    current_color = paletteToRGB332(color_index);
                    currentDrawColor = RGB332ToRGB565(current_color);
                }
                break;
                
            case DRAW_STROKE_POLYGON:
            case OPTIMIZED_POLYGON:
            case HOLLOW_POLYGON:
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 3 && numPoints <= 32) // Limit polygon complexity
                    {
                        // Allocate memory for polygon data using pool
                        size_t cmdDataSize = sizeof(int) * (1 + numPoints * 2);
                        int* polygonData = static_cast<int*>(poolAllocate(cmdDataSize));
                        if (polygonData) {
                            polygonData[0] = numPoints;
                            
                            // Read polygon vertices
                            for (uint32_t i = 0; i < numPoints; i++) {
                                const int32_t x = readZigzag(data, offset, dataSize);
                                const int32_t y = readZigzag(data, offset, dataSize);
                                polygonData[1 + i] = uint16ToPixel(x);
                                polygonData[1 + numPoints + i] = uint16ToPixel(y);
                            }
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, polygonData, cmdDataSize};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
                
            case DRAW_STROKE_POLYGONS:
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    int32_t accumX = 0, accumY = 0;
                    if (numPoints >= 3 && numPoints <= 32) // Limit polygon complexity
                    {
                        // Allocate memory for polygon data using pool
                        size_t cmdDataSize = sizeof(int) * (1 + numPoints * 2);
                        int* polygonData = static_cast<int*>(poolAllocate(cmdDataSize));
                        if (polygonData) {
                            polygonData[0] = numPoints;
                            
                            // Read polygon vertices with delta encoding
                            for (uint32_t i = 0; i < numPoints; i++) {
                                const int32_t deltaX = readZigzag(data, offset, dataSize);
                                const int32_t deltaY = readZigzag(data, offset, dataSize);
                                accumX += deltaX;
                                accumY += deltaY;
                                polygonData[1 + i] = uint16ToPixel(accumX);
                                polygonData[1 + numPoints + i] = uint16ToPixel(accumY);
                            }
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, polygonData, cmdDataSize};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
                
            case RECTANGLE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t dx = readZigzag(data, offset, dataSize);
                    const int32_t dy = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int pdx = uint16ToPixel(dx);
                    const int pdy = uint16ToPixel(dy);
                    
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE && 
                        pdx > 0 && pdy > 0 && !isPointOnMargin(px1, py1)) 
                    {
                        // Allocate memory for rectangle data using pool
                        int* rectData = static_cast<int*>(poolAllocate(sizeof(int) * 4));
                        if (rectData) {
                            rectData[0] = px1;
                            rectData[1] = py1;
                            rectData[2] = pdx;
                            rectData[3] = pdy;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, rectData, sizeof(int) * 4};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case SIMPLE_RECTANGLE:
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
                        pwidth > 0 && pheight > 0 && px + pwidth <= TILE_SIZE && py + pheight <= TILE_SIZE) 
                    {
                        // Allocate memory for rectangle data using pool
                        int* rectData = static_cast<int*>(poolAllocate(sizeof(int) * 4));
                        if (rectData) {
                            rectData[0] = px;
                            rectData[1] = py;
                            rectData[2] = pwidth;
                            rectData[3] = pheight;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, rectData, sizeof(int) * 4};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
                
            case CIRCLE:
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
                        // Allocate memory for circle data using pool
                        int* circleData = static_cast<int*>(poolAllocate(sizeof(int) * 3));
                        if (circleData) {
                            circleData[0] = pcx;
                            circleData[1] = pcy;
                            circleData[2] = pradius;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, circleData, sizeof(int) * 3};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case SIMPLE_CIRCLE:
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
                        // Allocate memory for circle data using pool
                        int* circleData = static_cast<int*>(poolAllocate(sizeof(int) * 3));
                        if (circleData) {
                            circleData[0] = pcx;
                            circleData[1] = pcy;
                            circleData[2] = pradius;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, circleData, sizeof(int) * 3};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case OPTIMIZED_CIRCLE:
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
                        // Allocate memory for circle data using pool
                        int* circleData = static_cast<int*>(poolAllocate(sizeof(int) * 3));
                        if (circleData) {
                            circleData[0] = pcx;
                            circleData[1] = pcy;
                            circleData[2] = pradius;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, circleData, sizeof(int) * 3};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case SIMPLE_TRIANGLE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t x3 = readZigzag(data, offset, dataSize);
                    const int32_t y3 = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int px2 = uint16ToPixel(x2);
                    const int py2 = uint16ToPixel(y2);
                    const int px3 = uint16ToPixel(x3);
                    const int py3 = uint16ToPixel(y3);
                    
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE &&
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) 
                    {
                        // Allocate memory for triangle data using pool
                        int* triangleData = static_cast<int*>(poolAllocate(sizeof(int) * 6));
                        if (triangleData) {
                            triangleData[0] = px1;
                            triangleData[1] = py1;
                            triangleData[2] = px2;
                            triangleData[3] = py2;
                            triangleData[4] = px3;
                            triangleData[5] = py3;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, triangleData, sizeof(int) * 6};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case OPTIMIZED_TRIANGLE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t x3 = readZigzag(data, offset, dataSize);
                    const int32_t y3 = readZigzag(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int px2 = uint16ToPixel(x2);
                    const int py2 = uint16ToPixel(y2);
                    const int px3 = uint16ToPixel(x3);
                    const int py3 = uint16ToPixel(y3);
                    
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE &&
                        px3 >= 0 && px3 <= TILE_SIZE && py3 >= 0 && py3 <= TILE_SIZE) 
                    {
                        // Allocate memory for triangle data using pool
                        int* triangleData = static_cast<int*>(poolAllocate(sizeof(int) * 6));
                        if (triangleData) {
                            triangleData[0] = px1;
                            triangleData[1] = py1;
                            triangleData[2] = px2;
                            triangleData[3] = py2;
                            triangleData[4] = px3;
                            triangleData[5] = py3;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, triangleData, sizeof(int) * 6};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case DASHED_LINE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t dashLength = readVarint(data, offset, dataSize);
                    const int32_t gapLength = readVarint(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int px2 = uint16ToPixel(x2);
                    const int py2 = uint16ToPixel(y2);
                    
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) 
                    {
                        // Allocate memory for dashed line data using pool
                        int* lineData = static_cast<int*>(poolAllocate(sizeof(int) * 6));
                        if (lineData) {
                            lineData[0] = px1;
                            lineData[1] = py1;
                            lineData[2] = px2;
                            lineData[3] = py2;
                            lineData[4] = dashLength;
                            lineData[5] = gapLength;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, lineData, sizeof(int) * 6};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case DOTTED_LINE:
                {
                    const int32_t x1 = readZigzag(data, offset, dataSize);
                    const int32_t y1 = readZigzag(data, offset, dataSize);
                    const int32_t x2 = readZigzag(data, offset, dataSize);
                    const int32_t y2 = readZigzag(data, offset, dataSize);
                    const int32_t dotSpacing = readVarint(data, offset, dataSize);
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int px2 = uint16ToPixel(x2);
                    const int py2 = uint16ToPixel(y2);
                    
                    if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                        px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) 
                    {
                        // Allocate memory for dotted line data using pool
                        int* lineData = static_cast<int*>(poolAllocate(sizeof(int) * 5));
                        if (lineData) {
                            lineData[0] = px1;
                            lineData[1] = py1;
                            lineData[2] = px2;
                            lineData[3] = py2;
                            lineData[4] = dotSpacing;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, lineData, sizeof(int) * 5};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case GRID_PATTERN:
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t width = readVarint(data, offset, dataSize);
                    const int32_t spacing = readVarint(data, offset, dataSize);
                    const int32_t count = readVarint(data, offset, dataSize);
                    const int32_t direction = readVarint(data, offset, dataSize);
                    const int px = uint16ToPixel(x);
                    const int py = uint16ToPixel(y);
                    
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE) 
                    {
                        // Allocate memory for grid pattern data using pool
                        int* gridData = static_cast<int*>(poolAllocate(sizeof(int) * 6));
                        if (gridData) {
                            gridData[0] = px;
                            gridData[1] = py;
                            gridData[2] = width;
                            gridData[3] = spacing;
                            gridData[4] = count;
                            gridData[5] = direction;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, gridData, sizeof(int) * 6};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
            case BLOCK_PATTERN:
                {
                    const int32_t x = readZigzag(data, offset, dataSize);
                    const int32_t y = readZigzag(data, offset, dataSize);
                    const int32_t blockSize = readVarint(data, offset, dataSize);
                    const int32_t spacing = readVarint(data, offset, dataSize);
                    const int32_t count = readVarint(data, offset, dataSize);
                    const int px = uint16ToPixel(x);
                    const int py = uint16ToPixel(y);
                    
                    if (px >= 0 && px <= TILE_SIZE && py >= 0 && py <= TILE_SIZE) 
                    {
                        // Allocate memory for block pattern data using pool
                        int* blockData = static_cast<int*>(poolAllocate(sizeof(int) * 5));
                        if (blockData) {
                            blockData[0] = px;
                            blockData[1] = py;
                            blockData[2] = blockSize;
                            blockData[3] = spacing;
                            blockData[4] = count;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, blockData, sizeof(int) * 5};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
                
            case DRAW_LINE:
            case DRAW_POLYLINE:
            case DRAW_HORIZONTAL_LINE:
            case DRAW_VERTICAL_LINE:
            case STRAIGHT_LINE:
            case COMPRESSED_POLYLINE:
            case PREDICTED_LINE:
            case RELATIVE_MOVE:
            case HIGHWAY_SEGMENT:
                {
                    // Read line data based on type
                    int32_t x1, y1, x2, y2;
                    
                    if (cmdType == DRAW_LINE) {
                        x1 = readZigzag(data, offset, dataSize);
                        y1 = readZigzag(data, offset, dataSize);
                        const int32_t dx = readZigzag(data, offset, dataSize);
                        const int32_t dy = readZigzag(data, offset, dataSize);
                        x2 = x1 + dx;
                        y2 = y1 + dy;
                    } else if (cmdType == DRAW_HORIZONTAL_LINE) {
                        x1 = readZigzag(data, offset, dataSize);
                        const int32_t dx = readZigzag(data, offset, dataSize);
                        y1 = readZigzag(data, offset, dataSize);
                        x2 = x1 + dx;
                        y2 = y1;
                    } else if (cmdType == DRAW_VERTICAL_LINE) {
                        x1 = readZigzag(data, offset, dataSize);
                        y1 = readZigzag(data, offset, dataSize);
                        const int32_t dy = readZigzag(data, offset, dataSize);
                        x2 = x1;
                        y2 = y1 + dy;
                    } else if (cmdType == STRAIGHT_LINE) {
                        x1 = readZigzag(data, offset, dataSize);
                        y1 = readZigzag(data, offset, dataSize);
                        const int32_t dx = readZigzag(data, offset, dataSize);
                        const int32_t dy = readZigzag(data, offset, dataSize);
                        x2 = x1 + dx;
                        y2 = y1 + dy;
                    } else if (cmdType == PREDICTED_LINE) {
                        // For predicted lines, we need to read startX, startY, patternType, length
                        const int32_t startX = readZigzag(data, offset, dataSize);
                        const int32_t startY = readZigzag(data, offset, dataSize);
                        const int32_t patternType = readVarint(data, offset, dataSize);
                        const int32_t length = readVarint(data, offset, dataSize);
                        
                        const int pstartX = uint16ToPixel(startX);
                        const int pstartY = uint16ToPixel(startY);
                        
                        if (pstartX >= 0 && pstartX <= TILE_SIZE && pstartY >= 0 && pstartY <= TILE_SIZE) 
                        {
                            // Allocate memory for predicted line data using pool
                            int* lineData = static_cast<int*>(poolAllocate(sizeof(int) * 4));
                            if (lineData) {
                                lineData[0] = pstartX;
                                lineData[1] = pstartY;
                                lineData[2] = patternType;
                                lineData[3] = length;
                                
                                // Determine layer and add command
                                RenderLayer layer = determineCommandLayer(cmdType, current_color);
                                RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, lineData, sizeof(int) * 4};
                                layerCommands[currentLayer].push_back(cmd);
                                executed++;
                            }
                        }
                        continue; // Skip the normal line processing below
                    } else if (cmdType == RELATIVE_MOVE) {
                        // For relative move, we need to read dx, dy and update current position
                        const int32_t dx = readZigzag(data, offset, dataSize);
                        const int32_t dy = readZigzag(data, offset, dataSize);
                        
                        // Update current position (this is handled by the caller)
                        // RELATIVE_MOVE doesn't create a render command, it just updates state
                        executed++;
                        continue; // Skip the normal line processing below
                    } else if (cmdType == HIGHWAY_SEGMENT) {
                        // For highway segments, we need to read startX, startY, endX, endY, width
                        const int32_t startX = readZigzag(data, offset, dataSize);
                        const int32_t startY = readZigzag(data, offset, dataSize);
                        const int32_t endX = readZigzag(data, offset, dataSize);
                        const int32_t endY = readZigzag(data, offset, dataSize);
                        const int32_t width = readVarint(data, offset, dataSize);
                        
                        const int pstartX = uint16ToPixel(startX);
                        const int pstartY = uint16ToPixel(startY);
                        const int pendX = uint16ToPixel(endX);
                        const int pendY = uint16ToPixel(endY);
                        
                        if (pstartX >= 0 && pstartX <= TILE_SIZE && pstartY >= 0 && pstartY <= TILE_SIZE &&
                            pendX >= 0 && pendX <= TILE_SIZE && pendY >= 0 && pendY <= TILE_SIZE) 
                        {
                            // Allocate memory for highway segment data using pool
                            int* segmentData = static_cast<int*>(poolAllocate(sizeof(int) * 5));
                            if (segmentData) {
                                segmentData[0] = pstartX;
                                segmentData[1] = pstartY;
                                segmentData[2] = pendX;
                                segmentData[3] = pendY;
                                segmentData[4] = width;
                                
                                // Determine layer and add command
                                RenderLayer layer = determineCommandLayer(cmdType, current_color);
                                RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, segmentData, sizeof(int) * 5};
                                layerCommands[currentLayer].push_back(cmd);
                                executed++;
                            }
                        }
                        continue; // Skip the normal line processing below
                    } else if (cmdType == COMPRESSED_POLYLINE) {
                        // For compressed polylines, we need to handle multiple points
                        const int32_t numPoints = readVarint(data, offset, dataSize);
                        if (numPoints < 2) continue;
                        
                        // Read first point
                        x1 = readZigzag(data, offset, dataSize);
                        y1 = readZigzag(data, offset, dataSize);
                        
                        // Read remaining points and draw lines
                        for (int i = 1; i < numPoints; i++) {
                            x2 = readZigzag(data, offset, dataSize);
                            y2 = readZigzag(data, offset, dataSize);
                            
                            const int px1 = uint16ToPixel(x1);
                            const int py1 = uint16ToPixel(y1);
                            const int px2 = uint16ToPixel(x2);
                            const int py2 = uint16ToPixel(y2);
                            
                            if (px1 >= 0 && px1 <= TILE_SIZE && py1 >= 0 && py1 <= TILE_SIZE &&
                                px2 >= 0 && px2 <= TILE_SIZE && py2 >= 0 && py2 <= TILE_SIZE) 
                            {
                                // Allocate memory for line data using pool
                                int* lineData = static_cast<int*>(poolAllocate(sizeof(int) * 4));
                                if (lineData) {
                                    lineData[0] = px1;
                                    lineData[1] = py1;
                                    lineData[2] = px2;
                                    lineData[3] = py2;
                                    
                                    // Determine layer and add command
                                    RenderLayer layer = determineCommandLayer(cmdType, current_color);
                                    RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, lineData, sizeof(int) * 4};
                                    layerCommands[currentLayer].push_back(cmd);
                                    executed++;
                                }
                            }
                            
                            // Move to next point
                            x1 = x2;
                            y1 = y2;
                        }
                        continue; // Skip the normal line processing below
                    } else {
                        // DRAW_POLYLINE - skip for now, complex to handle
                        continue;
                    }
                    
                    const int px1 = uint16ToPixel(x1);
                    const int py1 = uint16ToPixel(y1);
                    const int px2 = uint16ToPixel(x2);
                    const int py2 = uint16ToPixel(y2);
                    
                    if (shouldDrawLine(px1, py1, px2, py2)) 
                    {
                        // Allocate memory for line data using pool
                        int* lineData = static_cast<int*>(poolAllocate(sizeof(int) * 4));
                        if (lineData) {
                            lineData[0] = px1;
                            lineData[1] = py1;
                            lineData[2] = px2;
                            lineData[3] = py2;
                            
                            // Use current layer for command
                            RenderCommand cmd = {static_cast<uint8_t>(cmdType), current_color, currentDrawColor, lineData, sizeof(int) * 4};
                            layerCommands[currentLayer].push_back(cmd);
                            executed++;
                        }
                    }
                }
                break;
                
            default:
                // Skip unknown commands
                if (offset < dataSize - 4) 
                    offset += 4;
                break;
        }
    }

    // Second pass: render layers in correct order
    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        if (!layerCommands[layer].empty()) {
            renderLayer(map, layerCommands[layer], xOffset, yOffset);
            layerRenderCount++;
        }
    }

    // Clean up efficiently
    poolDeallocate(data);
    
    // Free all command data and clear layers
    for (int i = 0; i < LAYER_COUNT; i++) {
        for (auto& cmd : layerCommands[i]) {
            if (cmd.data) {
                poolDeallocate(cmd.data);
            }
        }
        layerCommands[i].clear();
    }

    if (executed == 0) 
        return false;

    // Add successfully rendered tile to cache
    addToCache(path, map);

    // Log rendering performance
    unsigned long renderTime = millis() - renderStart;
    ESP_LOGI(TAG, "Layered tile rendered: %s in %lums, %d commands, %d layers", 
             path, renderTime, executed, layerRenderCount);

    return true;
}

/**
 * @brief Calculate polygon bounding box
 *
 * @details Calculates the bounding box for a polygon defined by vertex arrays.
 *
 * @param px Array of x-coordinates of the polygon vertices.
 * @param py Array of y-coordinates of the polygon vertices.
 * @param numPoints The number of vertices in the polygon.
 * @param bounds Reference to store the calculated bounds.
 */
void Maps::calculatePolygonBounds(const int *px, const int *py, int numPoints, PolygonBounds& bounds)
{
    if (numPoints < 3) {
        bounds.isValid = false;
        return;
    }
    
    bounds.minX = bounds.maxX = px[0];
    bounds.minY = bounds.maxY = py[0];
    
    for (int i = 1; i < numPoints; ++i) {
        if (px[i] < bounds.minX) bounds.minX = px[i];
        if (px[i] > bounds.maxX) bounds.maxX = px[i];
        if (py[i] < bounds.minY) bounds.minY = py[i];
        if (py[i] > bounds.maxY) bounds.maxY = py[i];
    }
    
    bounds.isValid = true;
}

/**
 * @brief Check if polygon is in viewport
 *
 * @details Checks if the polygon's bounding box intersects with the visible area.
 *
 * @param bounds The polygon's bounding box.
 * @param xOffset The x-offset for positioning.
 * @param yOffset The y-offset for positioning.
 * @return true if polygon is in viewport, false otherwise.
 */
bool Maps::isPolygonInViewport(const PolygonBounds& bounds, int xOffset, int yOffset)
{
    if (!bounds.isValid) return false;
    
    int screenMinX = xOffset;
    int screenMinY = yOffset;
    int screenMaxX = xOffset + TILE_SIZE;
    int screenMaxY = yOffset + TILE_SIZE;
    
    // Check if bounding box intersects with screen area
    return !(bounds.maxX < screenMinX || bounds.minX > screenMaxX ||
             bounds.maxY < screenMinY || bounds.minY > screenMaxY);
}

/**
 * @brief Check if polygon can use optimized rendering
 *
 * @details Determines if a polygon is simple enough to use optimized rendering algorithms.
 *
 * @param px Array of x-coordinates of the polygon vertices.
 * @param py Array of y-coordinates of the polygon vertices.
 * @param numPoints The number of vertices in the polygon.
 * @return true if polygon can use optimized rendering, false otherwise.
 */
bool Maps::isSimplePolygon(const int *px, const int *py, int numPoints)
{
    if (numPoints < 3) return false;
    
    // Check for triangles (3 points)
    if (numPoints == 3) return true;
    
    // Check for rectangles (4 points with right angles)
    if (numPoints == 4) {
        // Simple rectangle check: opposite sides should be parallel
        int dx1 = px[1] - px[0];
        int dy1 = py[1] - py[0];
        int dx2 = px[2] - px[1];
        int dy2 = py[2] - py[1];
        
        // Check if sides are perpendicular (dot product = 0)
        if (dx1 * dx2 + dy1 * dy2 == 0) {
            return true;
        }
    }
    
    // For other polygons, use optimized scanline if enabled
    return optimizedScanlineEnabled;
}

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
    if (!polygonCullingEnabled && !optimizedScanlineEnabled) {
        fillPolygonGeneral(map, px, py, numPoints, color, xOffset, yOffset);
        return;
    }
    
    // Apply viewport culling if enabled
    if (polygonCullingEnabled) {
        PolygonBounds bounds;
        calculatePolygonBounds(px, py, numPoints, bounds);
        
        if (!isPolygonInViewport(bounds, xOffset, yOffset)) {
            polygonCulledCount++;
            return; // Skip rendering
        }
    }
    
    // Choose appropriate rendering algorithm
    if (isSimplePolygon(px, py, numPoints)) {
        polygonOptimizedCount++;
        
        if (numPoints == 3) {
            // Triangle
            fillTriangleOptimized(map, px[0] + xOffset, py[0] + yOffset,
                                 px[1] + xOffset, py[1] + yOffset,
                                 px[2] + xOffset, py[2] + yOffset, color);
        } else if (numPoints == 4) {
            // Rectangle
            PolygonBounds bounds;
            calculatePolygonBounds(px, py, numPoints, bounds);
            fillRectangleOptimized(map, bounds.minX + xOffset, bounds.minY + yOffset,
                                  bounds.maxX - bounds.minX, bounds.maxY - bounds.minY, color);
        } else {
            // Complex polygon with optimized scanline
            if (optimizedScanlineEnabled) {
                fillPolygonScanlineOptimized(map, px, py, numPoints, color, xOffset, yOffset);
            } else {
                fillPolygonGeneral(map, px, py, numPoints, color, xOffset, yOffset);
            }
        }
    } else {
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
    for (int y = y1; y < y2; y++) {
        int x_start = x1 + (y - y1) * dx1 / dy1;
        int x_end = x1 + (y - y1) * dx2 / dy2;
        
        if (x_start > x_end) std::swap(x_start, x_end);
        
        if (x_start >= 0 && x_end < TILE_SIZE && y >= 0 && y < TILE_SIZE) {
            map.drawLine(x_start, y, x_end, y, color);
        }
    }
    
    // Fill bottom half
    for (int y = y2; y <= y3; y++) {
        int x_start = x2 + (y - y2) * dx3 / dy3;
        int x_end = x1 + (y - y1) * dx2 / dy2;
        
        if (x_start > x_end) std::swap(x_start, x_end);
        
        if (x_start >= 0 && x_end < TILE_SIZE && y >= 0 && y < TILE_SIZE) {
            map.drawLine(x_start, y, x_end, y, color);
        }
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
    
    if (w > 0 && h > 0) {
        map.fillRect(x, y, w, h, color);
    }
}

/**
 * @brief Optimized scanline algorithm
 *
 * @details Fills a polygon using an optimized scanline algorithm with better memory management.
 *
 * @param map The TFT_eSprite object where the polygon will be drawn.
 * @param px Array of x-coordinates of the polygon vertices.
 * @param py Array of y-coordinates of the polygon vertices.
 * @param numPoints The number of vertices in the polygon.
 * @param color The color to fill the polygon with.
 * @param xOffset The x-offset to apply when drawing the polygon.
 * @param yOffset The y-offset to apply when drawing the polygon.
 */
void Maps::fillPolygonScanlineOptimized(TFT_eSprite &map, const int *px, const int *py, int numPoints, uint16_t color, int xOffset, int yOffset)
{
    // Calculate bounding box
    PolygonBounds bounds;
    calculatePolygonBounds(px, py, numPoints, bounds);
    
    if (!bounds.isValid) return;
    
    // Clamp to tile boundaries
    int minY = std::max(bounds.minY, 0);
    int maxY = std::min(bounds.maxY, TILE_SIZE - 1);
    
    // Optimized scanline algorithm with better performance
    for (int y = minY; y <= maxY; y++) {
        int intersections[32]; // Stack-allocated for small polygons
        int count = 0;
        
        // Find intersections with current scanline
        for (int i = 0; i < numPoints; i++) {
            int j = (i + 1) % numPoints;
            
            int y1 = py[i];
            int y2 = py[j];
            int x1 = px[i];
            int x2 = px[j];
            
            // Check if scanline intersects this edge (optimized condition)
            if ((y1 < y && y <= y2) || (y2 < y && y <= y1)) {
                // Calculate intersection X coordinate with fixed-point arithmetic
                int x;
                if (y1 == y2) {
                    x = x1; // Horizontal edge
                } else {
                    // Use fixed-point arithmetic for better precision
                    int dy = y2 - y1;
                    int dx = x2 - x1;
                    x = x1 + ((y - y1) * dx) / dy;
                }
                intersections[count++] = x;
            }
        }
        
        // Sort intersections (use insertion sort for small arrays)
        for (int i = 1; i < count; i++) {
            int key = intersections[i];
            int j = i - 1;
            while (j >= 0 && intersections[j] > key) {
                intersections[j + 1] = intersections[j];
                j--;
            }
            intersections[j + 1] = key;
        }
        
        // Fill between pairs of intersections
        for (int i = 0; i < count - 1; i += 2) {
            int x1 = std::max(0, intersections[i] + xOffset);
            int x2 = std::min(TILE_SIZE - 1, intersections[i + 1] + xOffset);
            int yy = y + yOffset;
            
            if (x1 < x2 && yy >= 0 && yy < TILE_SIZE + yOffset) {
                map.drawFastHLine(x1, yy, x2 - x1 + 1, color);
            }
        }
    }
}

/**
 * @brief Print polygon optimization statistics
 *
 * @details Prints useful polygon optimization statistics for debugging and monitoring.
 */
void Maps::printPolygonStats()
{
    ESP_LOGI(TAG, "=== Polygon Optimization Statistics ===");
    ESP_LOGI(TAG, "Culling enabled: %s", polygonCullingEnabled ? "yes" : "no");
    ESP_LOGI(TAG, "Optimized scanline enabled: %s", optimizedScanlineEnabled ? "yes" : "no");
    ESP_LOGI(TAG, "Total polygons rendered: %u", polygonRenderCount);
    ESP_LOGI(TAG, "Polygons culled: %u", polygonCulledCount);
    ESP_LOGI(TAG, "Polygons optimized: %u", polygonOptimizedCount);
    
    if (polygonRenderCount > 0) {
        float cullRate = (float)polygonCulledCount / polygonRenderCount * 100.0f;
        float optRate = (float)polygonOptimizedCount / polygonRenderCount * 100.0f;
        ESP_LOGI(TAG, "Culling rate: %.1f%%", cullRate);
        ESP_LOGI(TAG, "Optimization rate: %.1f%%", optRate);
    }
    ESP_LOGI(TAG, "=====================================");
}

/**
 * @brief Initialize polygon optimization system (public method)
 *
 * @details Public method to initialize the polygon optimization system.
 */
void Maps::initializePolygonOptimizations()
{
    initPolygonOptimizations();
}

/**
 * @brief Print polygon optimization statistics (public method)
 *
 * @details Public method to print polygon optimization statistics.
 */
void Maps::printPolygonOptimizationStats()
{
    printPolygonStats();
}

/**
 * @brief Enable/disable polygon culling (public method)
 *
 * @details Public method to enable or disable polygon culling.
 *
 * @param enable true to enable culling, false to disable.
 */
void Maps::enablePolygonCulling(bool enable)
{
    polygonCullingEnabled = enable;
    ESP_LOGI(TAG, "Polygon culling %s", enable ? "enabled" : "disabled");
}

/**
 * @brief Enable/disable optimized scanline (public method)
 *
 * @details Public method to enable or disable optimized scanline algorithm.
 *
 * @param enable true to enable optimized scanline, false to disable.
 */
void Maps::enableOptimizedScanline(bool enable)
{
    optimizedScanlineEnabled = enable;
    ESP_LOGI(TAG, "Optimized scanline %s", enable ? "enabled" : "disabled");
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
    if (getCachedTile(path, map, xOffset, yOffset)) {
        return true; // Tile found in cache
    }

    // Use original rendering with unified pool


    FILE* file = fopen(path, "rb");
    if (!file)
       return false;
    
    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = static_cast<uint8_t*>(unifiedAlloc(fileSize, 0)); // Type 0 = general
    if (!data)
    {
        fclose(file);
        return false;
    }
    if (!unifiedPoolLogged) {
        ESP_LOGI(TAG, "renderTile: Using UNIFIED POOL for data allocation");
        unifiedPoolLogged = true;
    }

    const size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) 
    {
        unifiedDealloc(data);
        return false;
    }

    // Update memory statistics before rendering
    updateMemoryStats();

    size_t offset = 0;
    const size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = RGB332ToRGB565(current_color);

    const uint32_t num_cmds = readVarint(data, offset, dataSize);
    if (num_cmds == 0)
    {
        unifiedDealloc(data);
        return false;
    }

    // Use optimal batch size for this hardware
    const size_t optimalBatchSize = getOptimalBatchSize();
    
    // Use unified pool for line batch allocation
    LineSegment* lineBatch = static_cast<LineSegment*>(unifiedAlloc(optimalBatchSize * sizeof(LineSegment), 5)); // Type 5 = lineSegment
    if (!lineBatch) {
        ESP_LOGW(TAG, "renderTile: Unified pool allocation failed for lineBatch");
        unifiedDealloc(data);
        return false;
    }
    if (!unifiedLineBatchLogged) {
        ESP_LOGI(TAG, "renderTile: Using UNIFIED POOL for lineBatch allocation");
        unifiedLineBatchLogged = true;
    }
    unifiedPoolHitCount++;
    
    int batchCount = 0;
    if (!lineBatch) 
    {
        unifiedDealloc(data);
        return false;
    }

    // Track memory allocations for statistics
    totalMemoryAllocations++;

    int executed = 0;
    int totalLines = 0;
    int batchFlushes = 0;
    unsigned long renderStart = millis();
    
    // Command type counters for debugging
    int lineCommands = 0;
    int polygonCommands = 0;
    int rectangleCommands = 0;
    int circleCommands = 0;

    // Optimized flushBatch with better memory access patterns
    auto flushBatch = [&]() 
    {
        if (batchCount == 0) return;
        
        totalLines += batchCount;
        batchFlushes++;
        
        // Batch draw lines with optimized memory access
        for (int i = 0; i < batchCount; ++i) 
        {
            const LineSegment& segment = lineBatch[i];
            map.drawLine(segment.x0, segment.y0, segment.x1, segment.y1, segment.color);
        }
        batchCount = 0;
    };

    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) 
    {
        if (offset >= dataSize) 
            break;
        const size_t cmdStartOffset = offset;
        const uint32_t cmdType = readVarint(data, offset, dataSize);

        bool isLineCommand = false;

        switch (cmdType) 
        {
            case SET_LAYER:
                flushBatch();
                {
                    const int32_t layerNumber = readVarint(data, offset, dataSize);
                    // SET_LAYER command - layer is already determined by the layer system
                    // This command is mainly for compatibility with tile_viewer.py
                    // We don't need to process it further as the layer system handles ordering
                    executed++;
                }
                break;
                
            case SET_COLOR:
                flushBatch();
                if (offset < dataSize) 
                {
                    current_color = data[offset++];
                    currentDrawColor = RGB332ToRGB565(current_color);
                    executed++;
                }
                break;
            case SET_COLOR_INDEX:
                flushBatch();
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
                        if (batchCount < optimalBatchSize) 
                            lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                        else 
                        {
                            flushBatch();
                            lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                        }
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
                        // Use unified pool directly
                        int* px = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        int* py = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        if (!px || !py) {
                            if (px) unifiedDealloc(px);
                            if (py) unifiedDealloc(py);
                            ESP_LOGW(TAG, "renderTile DRAW_POLYLINE: Unified pool allocation failed");
                            continue;
                        }
                        if (!unifiedPolylineLogged) {
                            ESP_LOGI(TAG, "renderTile DRAW_POLYLINE: Using UNIFIED POOL for coordinate arrays");
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
                                if (batchCount < optimalBatchSize) 
                                    lineBatch[batchCount++] = {px[i-1], py[i-1], px[i], py[i], currentDrawColor};
                                else 
                                {
                                    flushBatch();
                                    lineBatch[batchCount++] = {px[i-1], py[i-1], px[i], py[i], currentDrawColor};
                                }
                            }
                        }
                        executed++;
                        isLineCommand = true;
                        
                        // Always use unified deallocation since we use unified pool directly
                        unifiedDealloc(px);
                        unifiedDealloc(py);
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
                flushBatch();
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    if (numPoints >= 3) 
                    {
                        int* px = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        int* py = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        if (!px || !py) 
                        {
                            if (px) 
                                unifiedDealloc(px);
                            if (py) 
                                unifiedDealloc(py);
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i)
                            {
                                readZigzag(data, offset, dataSize);
                                readZigzag(data, offset, dataSize);
                            }
                            break;
                        }
                        if (!unifiedPolygonLogged) {
                            ESP_LOGI(TAG, "renderTile DRAW_STROKE_POLYGON: Using UNIFIED POOL for coordinate arrays");
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
                        unifiedDealloc(px);
                        unifiedDealloc(py);
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
                flushBatch();
                {
                    const uint32_t numPoints = readVarint(data, offset, dataSize);
                    int32_t accumX = 0, accumY = 0;
                    if (numPoints >= 3) 
                    {
                        int* px = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        int* py = static_cast<int*>(unifiedAlloc(numPoints * sizeof(int), 6)); // Type 6 = coordArray
                        if (!px || !py) 
                        {
                            if (px)
                                unifiedDealloc(px);
                            if (py) 
                                unifiedDealloc(py);
                            for (uint32_t i = 0; i < numPoints && offset < dataSize; ++i) 
                            {
                                readZigzag(data, offset, dataSize);
                                readZigzag(data, offset, dataSize);
                            }
                            break;
                        }
                        if (!unifiedPolygonsLogged) {
                            ESP_LOGI(TAG, "renderTile DRAW_STROKE_POLYGONS: Using UNIFIED POOL for coordinate arrays");
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
                        unifiedDealloc(px);
                        unifiedDealloc(py);
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
                        if (batchCount < optimalBatchSize)
                            lineBatch[batchCount++] = {px1, py, px2, py, currentDrawColor};
                        else
                        {
                            flushBatch();
                            lineBatch[batchCount++] = {px1, py, px2, py, currentDrawColor};
                        }
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
                        if (batchCount < optimalBatchSize) 
                            lineBatch[batchCount++] = {px, py1, px, py2, currentDrawColor};
                        else 
                        {
                            flushBatch();
                            lineBatch[batchCount++] = {px, py1, px, py2, currentDrawColor};
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case RECTANGLE:
                flushBatch();
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
                            drawFilledRectWithBorder(map, px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor, borderColor, fillPolygons);
                            executed++;
                        }
                        else
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case SIMPLE_RECTANGLE:
                flushBatch();
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
                            // Fill rectangle with border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            drawFilledRectWithBorder(map, px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor, borderColor, fillPolygons);
                        }
                        else
                        {
                            // Draw only outline
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                        }
                        executed++;
                    }
                }
                break;
            case OPTIMIZED_RECTANGLE:
                flushBatch();
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
                            // Fill rectangle with border
                            const uint16_t borderColor = RGB332ToRGB565(darkenRGB332(current_color));
                            drawFilledRectWithBorder(map, px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor, borderColor, fillPolygons);
                        }
                        else
                        {
                            // Draw only outline
                            map.drawRect(px + xOffset, py + yOffset, pwidth, pheight, currentDrawColor);
                        }
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
                        if (batchCount < optimalBatchSize)
                            lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                        else
                        {
                            flushBatch();
                            lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case CIRCLE:
                flushBatch();
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
                            drawFilledCircleWithBorder(map, pcx + xOffset, pcy + yOffset, pradius, currentDrawColor, borderColor, fillPolygons);
                            executed++;
                        }
                        else
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);

                        executed++;
                    }
                }
                break;
            case SIMPLE_CIRCLE:
                flushBatch();
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
                            drawFilledCircleWithBorder(map, pcx + xOffset, pcy + yOffset, pradius, currentDrawColor, borderColor, fillPolygons);
                        }
                        else
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case SIMPLE_TRIANGLE:
                flushBatch();
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
                flushBatch();
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
                flushBatch();
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
                        drawDashedLine(map, px1, py1, px2, py2, dashLength, gapLength, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case DOTTED_LINE:
                flushBatch();
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
                        drawDottedLine(map, px1, py1, px2, py2, dotSpacing, currentDrawColor);
                        executed++;
                    }
                }
                break;
            case GRID_PATTERN:
                flushBatch();
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
                        drawGridPattern(map, px, py, width, spacing, count, direction, currentDrawColor);
                        executed++;
                    }
                }
                break;            
            default:
                flushBatch();
                if (offset < dataSize - 4) 
                    offset += 4;

                break;
        }
        if (!isLineCommand) 
            flushBatch();

        if (offset <= cmdStartOffset) 
            break;
    }
    flushBatch();

    // Use unified deallocation for better memory management
    unifiedDealloc(data);
    
    // Use unified deallocation for lineBatch
    unifiedDealloc(lineBatch);
    
    // Track memory deallocations for statistics
    totalMemoryDeallocations += 2;

    if (executed == 0) 
        return false;

    // Add successfully rendered tile to cache
    addToCache(path, map);

    // Log rendering performance
    unsigned long renderTime = millis() - renderStart;
    if (totalLines > 0) {
        ESP_LOGI(TAG, "Tile rendered: %s", path);
        ESP_LOGI(TAG, "Performance: %lu ms, %d lines, %d batches, avg %.1f lines/batch", 
                 renderTime, totalLines, batchFlushes, (float)totalLines / batchFlushes);
    }

    return true;
}

/**
 * @brief Enable/disable layer rendering
 *
 * @details Enables or disables the layer rendering system for proper map layer ordering.
 *
 * @param enable true to enable layer rendering, false to disable
 * @return bool Current state of layer rendering
 */
bool Maps::enableLayerRendering(bool enable)
{
    layerRenderingEnabled = enable;
    ESP_LOGI(TAG, "Layer rendering %s", enable ? "enabled" : "disabled");
    return layerRenderingEnabled;
}

/**
 * @brief Print layer rendering statistics
 *
 * @details Prints useful layer rendering statistics for debugging and monitoring.
 */
void Maps::printLayerRenderingStats()
{
    ESP_LOGI(TAG, "=== Layer Rendering Statistics ===");
    ESP_LOGI(TAG, "Layer rendering enabled: %s", layerRenderingEnabled ? "Yes" : "No");
    ESP_LOGI(TAG, "Total layers rendered: %u", layerRenderCount);
    
    const char* layerNames[] = {"Terrain", "Water", "Buildings", "Outlines", "Roads"};
    for (int i = 0; i < LAYER_COUNT; i++) {
        ESP_LOGI(TAG, "Layer %s: %zu commands", layerNames[i], layerCommands[i].size());
    }
    
    ESP_LOGI(TAG, "================================");
}


// Unified Memory Pool Implementation
void Maps::initUnifiedPool()
{
    ESP_LOGI(TAG, "Initializing unified memory pool...");
    
    if (unifiedPoolMutex == nullptr) {
        unifiedPoolMutex = xSemaphoreCreateMutex();
        if (unifiedPoolMutex == nullptr) {
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

void* Maps::unifiedAlloc(size_t size, uint8_t type)
{
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : unifiedPool) {
            if (!entry.isInUse && entry.size >= size) {
                entry.isInUse = true;
                entry.allocationCount++;
                entry.type = type;
                unifiedPoolHitCount++;
                xSemaphoreGive(unifiedPoolMutex);
                return entry.ptr;
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

void Maps::unifiedDealloc(void* ptr)
{
    if (!ptr) return;
    
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : unifiedPool) {
            if (entry.ptr == ptr && entry.isInUse) {
                entry.isInUse = false;
                xSemaphoreGive(unifiedPoolMutex);
                return;
            }
        }
        xSemaphoreGive(unifiedPoolMutex);
    }
    
    heap_caps_free(ptr);
}

void Maps::clearUnifiedPool()
{
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& entry : unifiedPool) {
            if (entry.ptr) {
                heap_caps_free(entry.ptr);
            }
        }
        unifiedPool.clear();
        unifiedPoolHitCount = 0;
        unifiedPoolMissCount = 0;
        xSemaphoreGive(unifiedPoolMutex);
        ESP_LOGI(TAG, "Unified memory pool cleared");
    }
}

void Maps::printUnifiedPoolStats()
{
    if (unifiedPoolMutex && xSemaphoreTake(unifiedPoolMutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Unified Pool Stats:");
        ESP_LOGI(TAG, "  Entries: %zu/%zu used", unifiedPool.size(), maxUnifiedPoolEntries);
        ESP_LOGI(TAG, "  Hits: %u, Misses: %u", unifiedPoolHitCount, unifiedPoolMissCount);
        
        uint32_t totalRequests = unifiedPoolHitCount + unifiedPoolMissCount;
        if (totalRequests > 0) {
            float hitRate = (float)unifiedPoolHitCount * 100.0f / totalRequests;
            ESP_LOGI(TAG, "  Hit Rate: %.1f%%", hitRate);
        }
        
        xSemaphoreGive(unifiedPoolMutex);
    }
}