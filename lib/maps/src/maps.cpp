/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.3
 * @date 2025-06
 */

#include "maps.hpp"
#include <vector>
#include <algorithm>
#include <algorithm>
#include <cstring>

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData; /**< Vector containing track waypoints */
const char* TAG PROGMEM = "Maps";


/**********************************************************/
/**********************************************************/
// Static member definitions
std::unordered_map<std::string, TileCache> Maps::tileCache;
bool Maps::hasPsram = false;
size_t Maps::maxCacheSize = Maps::MAX_CACHE_SIZE_NO_PSRAM;
bool Maps::debugMode = false;
int Maps::qualityLevel = 2; // Default: balanced
uint32_t Maps::cacheHits = 0;
uint32_t Maps::cacheMisses = 0;
uint32_t Maps::tilesRendered = 0;
uint32_t Maps::commandsExecuted = 0;

// TileCache implementation
TileCache::TileCache() : data(nullptr), size(0), lastAccess(0), inPsram(false) {}

TileCache::~TileCache() {
    if (data) {
        if (inPsram) {
            heap_caps_free(data);
        } else {
            delete[] data;
        }
        data = nullptr;
    }
}

TileCache::TileCache(TileCache&& other) noexcept 
    : data(other.data), size(other.size), lastAccess(other.lastAccess), inPsram(other.inPsram) {
    other.data = nullptr;
    other.size = 0;
    other.lastAccess = 0;
    other.inPsram = false;
}

TileCache& TileCache::operator=(TileCache&& other) noexcept {
    if (this != &other) {
        // Clean up existing data
        if (data) {
            if (inPsram) {
                heap_caps_free(data);
            } else {
                delete[] data;
            }
        }
        
        // Move data from other
        data = other.data;
        size = other.size;
        lastAccess = other.lastAccess;
        inPsram = other.inPsram;
        
        // Reset other
        other.data = nullptr;
        other.size = 0;
        other.lastAccess = 0;
        other.inPsram = false;
    }
    return *this;
}
/**********************************************************/
/**********************************************************/

/**
 * @brief Map Class constructor
 */
Maps::Maps() {}

// Render Map Private section

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
	return ((uint16_t)(((f_lon + 180.0f) / 360.0f * (float)(1 << zoom) * tileSize)) % tileSize);
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
    float lat_rad = f_lat * (float)M_PI / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);

    float scale = (1 << zoom) * tileSize;

    return (uint16_t)(((1.0f - merc_n / (float)M_PI) / 2.0f * scale)) % tileSize;
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
    return (uint32_t)(rawTile);
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
    float lat_rad = f_lat * M_PI / 180.0f;
    float siny = tanf(lat_rad) + 1.0f / cosf(lat_rad);
    float merc_n = logf(siny);

    float rawTile = (1.0f - merc_n / (float)M_PI) / 2.0f * (1 << zoom);
    rawTile += 1e-6f;
    return (uint32_t)(rawTile);
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
	return (float)tileX * 360.0f / (1 << zoom) - 180.0f;
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
	float scale = (float)(1 << zoom);
	float n = (float)M_PI * (1.0f - 2.0f * (float)tileY / scale);
	return 180.0f / (float)M_PI * atanf(sinhf(n));
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

// Public section

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
}

/**
 * @brief Delete map screen and release PSRAM
 *
 * @details Deletes the main map sprite to free up PSRAM.
 */
void Maps::deleteMapScrSprites()
{
 	Maps::mapSprite.deleteSprite();
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
		Maps::mapTempSprite.fillSprite(TFT_WHITE); 
		const int16_t size = Maps::mapTileSize;

		if (mapSet.vectorMap)
			Maps::isMapFound = renderTile(Maps::currentMapTile.file, size, size,Maps::mapTempSprite);
		else
			Maps::isMapFound = Maps::mapTempSprite.drawPngFile(Maps::currentMapTile.file, size, size);

		Maps::oldMapTile = Maps::currentMapTile;
		strcpy(Maps::oldMapTile.file, Maps::currentMapTile.file);

		if (!Maps::isMapFound)
		{
			ESP_LOGE(TAG, "No Map Found!");
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

		Maps::roundMapTile = Maps::getMapTile(
			Maps::currentMapTile.lon,
			Maps::currentMapTile.lat,
			Maps::zoomLevel,
			tileToLoadX,
			tileToLoadY
		);

		const int16_t offsetX = (dirX != 0) ? i * tileSize : 0;
		const int16_t offsetY = (dirY != 0) ? i * tileSize : 0;

		const bool foundTile = preloadSprite.drawPngFile(
			Maps::roundMapTile.file,
			offsetX,
			offsetY
		);

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


/***************************************************************************************************************/
/***************************************************************************************************************/
/***************************************************************************************************************/
/***************************************************************************************************************/
// Cache management
void Maps::initCache() {
    hasPsram = (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0);
    maxCacheSize = hasPsram ? MAX_CACHE_SIZE_PSRAM : MAX_CACHE_SIZE_NO_PSRAM;
    
    // Clear existing cache
    tileCache.clear();
    
    // Reset statistics
    cacheHits = 0;
    cacheMisses = 0;
    tilesRendered = 0;
    commandsExecuted = 0;
    
    ESP_LOGI(TAG, "Enhanced renderer initialized. PSRAM: %s, Max cache: %zu tiles, Quality: %d", 
             hasPsram ? "YES" : "NO", maxCacheSize, qualityLevel);
}

void Maps::clearCache() {
    tileCache.clear();
    ESP_LOGI(TAG, "Enhanced renderer cache cleared. Stats - Hits: %u, Misses: %u, Tiles: %u", 
             cacheHits, cacheMisses, tilesRendered);
    
    // Reset statistics
    cacheHits = 0;
    cacheMisses = 0;
    tilesRendered = 0;
    commandsExecuted = 0;
}

void Maps::printCacheStats() {
    size_t memoryUsed = getCacheUsage();
    float hitRatio = (cacheHits + cacheMisses) > 0 ? (100.0f * cacheHits / (cacheHits + cacheMisses)) : 0.0f;
    
    ESP_LOGI(TAG, "=== CACHE STATISTICS ===");
    ESP_LOGI(TAG, "Cached tiles: %zu/%zu", tileCache.size(), maxCacheSize);
    ESP_LOGI(TAG, "Memory used: %zu bytes (%.1f KB)", memoryUsed, memoryUsed / 1024.0f);
    ESP_LOGI(TAG, "Cache hits: %u, misses: %u (%.1f%% hit ratio)", cacheHits, cacheMisses, hitRatio);
    ESP_LOGI(TAG, "Tiles rendered: %u, commands executed: %u", tilesRendered, commandsExecuted);
    
    if (debugMode) {
        ESP_LOGI(TAG, "=== CACHE CONTENTS ===");
        for (const auto& entry : tileCache) {
            ESP_LOGI(TAG, "  %s: %zu bytes (%s)", 
                     entry.first.c_str(), 
                     entry.second.size,
                     entry.second.inPsram ? "PSRAM" : "RAM");
        }
    }
}

size_t Maps::getCacheUsage() {
    size_t total = 0;
    for (const auto& entry : tileCache) {
        total += entry.second.size;
    }
    return total;
}

void Maps::setDebugMode(bool enabled) {
    debugMode = enabled;
    ESP_LOGI(TAG, "Debug mode %s", enabled ? "enabled" : "disabled");
}

void Maps::setQualityLevel(int level) {
    if (level >= 1 && level <= 3) {
        qualityLevel = level;
        ESP_LOGI(TAG, "Quality level set to %d (%s)", level, 
                 (level == 1) ? "fast" : (level == 2) ? "balanced" : "quality");
    } else {
        ESP_LOGW(TAG, "Invalid quality level %d, keeping %d", level, qualityLevel);
    }
}

// Utility functions
bool Maps::validateTileData(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(uint16_t)) {
        ESP_LOGE(TAG, "Invalid tile data: null or too small (%zu bytes)", size);
        return false;
    }
    
    uint16_t numCommands = *(uint16_t*)data;
    if (numCommands > 15000) {  // Reasonable upper limit
        ESP_LOGE(TAG, "Suspicious command count: %d", numCommands);
        return false;
    }
    
    if (size > MAX_TILE_SIZE_BYTES) {
        ESP_LOGE(TAG, "Tile too large: %zu bytes (max %zu)", size, MAX_TILE_SIZE_BYTES);
        return false;
    }
    
    return true;
}

void Maps::debugPrintCommand(uint8_t cmdType, GeometryComplexity complexity) {
    if (!debugMode) return;
    
    const char* cmd_name;
    switch (cmdType) {
        case DRAW_LINE: cmd_name = "LINE"; break;
        case DRAW_POLYLINE: cmd_name = "POLYLINE"; break;
        case DRAW_FILL_RECT: cmd_name = "FILL_RECT"; break;
        case DRAW_FILL_POLYGON: cmd_name = "FILL_POLYGON"; break;
        case DRAW_STROKE_POLYGON: cmd_name = "STROKE_POLYGON"; break;
        case DRAW_ADAPTIVE_LINE: cmd_name = "ADAPTIVE_LINE"; break;
        case DRAW_SPLINE_CURVE: cmd_name = "SPLINE_CURVE"; break;
        case DRAW_MULTI_LOD_POLYGON: cmd_name = "MULTI_LOD_POLYGON"; break;
        case DRAW_HORIZONTAL_LINE: cmd_name = "H_LINE"; break;
        case DRAW_VERTICAL_LINE: cmd_name = "V_LINE"; break;
        default: cmd_name = "UNKNOWN"; break;
    }
    
    const char* complexity_name;
    switch (complexity) {
        case COMPLEXITY_LOW: complexity_name = "LOW"; break;
        case COMPLEXITY_MEDIUM: complexity_name = "MED"; break;
        case COMPLEXITY_HIGH: complexity_name = "HIGH"; break;
        default: complexity_name = "UNK"; break;
    }
    
    ESP_LOGI(TAG, "Command: %s (complexity=%s, quality=%d)", cmd_name, complexity_name, qualityLevel);
}

void Maps::evictOldestTile() {
    if (tileCache.empty()) return;
    
    auto oldest = tileCache.begin();
    uint32_t oldestTime = oldest->second.lastAccess;
    
    for (auto it = tileCache.begin(); it != tileCache.end(); ++it) {
        if (it->second.lastAccess < oldestTime) {
            oldest = it;
            oldestTime = it->second.lastAccess;
        }
    }
    
    if (debugMode) {
        ESP_LOGI(TAG, "Evicting tile from cache: %s", oldest->first.c_str());
    }
    
    tileCache.erase(oldest);
}

bool Maps::loadTileFromFile(const char* path, TileCache& cache) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > MAX_TILE_SIZE_BYTES) {
        ESP_LOGE(TAG, "Invalid file size: %ld bytes", fileSize);
        fclose(file);
        return false;
    }
    
    // Allocate memory (prefer PSRAM for larger tiles)
    if (hasPsram && fileSize > 1024) {
        cache.data = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM);
        cache.inPsram = true;
    } else {
        cache.data = new uint8_t[fileSize];
        cache.inPsram = false;
    }
    
    if (!cache.data) {
        ESP_LOGE(TAG, "Failed to allocate %ld bytes for tile", fileSize);
        fclose(file);
        return false;
    }
    
    // Read file data
    size_t bytesRead = fread(cache.data, 1, fileSize, file);
    fclose(file);
    
    if (bytesRead != fileSize) {
        ESP_LOGE(TAG, "Failed to read complete tile data: %zu/%ld bytes", bytesRead, fileSize);
        if (cache.inPsram) {
            heap_caps_free(cache.data);
        } else {
            delete[] cache.data;
        }
        cache.data = nullptr;
        return false;
    }
    
    cache.size = fileSize;
    cache.lastAccess = millis();
    
    if (debugMode) {
        ESP_LOGI(TAG, "Loaded tile: %ld bytes (%s)", fileSize, cache.inPsram ? "PSRAM" : "RAM");
    }
    
    return true;
}

// Main rendering function
bool Maps::renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!path) {
        ESP_LOGE(TAG, "Invalid tile path");
        return false;
    }
    
    // Initialize cache if not done yet
    static bool cacheInitialized = false;
    if (!cacheInitialized) {
        initCache();
        cacheInitialized = true;
    }
    
    // Try to get tile data from cache first
    auto cacheIt = tileCache.find(std::string(path));
    uint8_t* dataPtr = nullptr;
    size_t dataSize = 0;
    
    if (cacheIt != tileCache.end()) {
        // Cache hit
        cacheHits++;
        cacheIt->second.lastAccess = millis();
        dataPtr = cacheIt->second.data;
        dataSize = cacheIt->second.size;
        
        if (debugMode) {
            ESP_LOGI(TAG, "Cache hit for tile: %s", path);
        }
    } else {
        // Cache miss - load from file
        cacheMisses++;
        
        if (debugMode) {
            ESP_LOGI(TAG, "Loading tile from file: %s", path);
        }
        
        // Check if we need to make room in cache
        if (tileCache.size() >= maxCacheSize) {
            evictOldestTile();
        }
        
        // Create new cache entry
        TileCache newCache;
        if (!loadTileFromFile(path, newCache)) {
            if (debugMode) {
                ESP_LOGW(TAG, "Failed to load tile: %s", path);
            }
            return false;
        }
        
        dataPtr = newCache.data;
        dataSize = newCache.size;
        
        // Move cache into map
        tileCache[std::string(path)] = std::move(newCache);
    }
    
    // Validate tile data
    if (!validateTileData(dataPtr, dataSize)) {
        ESP_LOGE(TAG, "Invalid tile data: %s", path);
        return false;
    }
    
    // Parse and execute commands
    size_t offset = 0;
    uint16_t numCommands = *(uint16_t*)(dataPtr + offset);
    offset += sizeof(uint16_t);
    
    if (debugMode) {
        ESP_LOGI(TAG, "Processing tile %s: %d commands", path, numCommands);
    }
    
    // Handle empty tiles
    if (numCommands == 0) {
        if (debugMode) {
            ESP_LOGI(TAG, "Empty tile: %s", path);
        }
        tilesRendered++;
        return true;
    }
    
    int executedCommands = 0;
    int skippedCommands = 0;
    
    // Process each command
    for (int i = 0; i < numCommands && offset < dataSize; ++i) {
        if (offset >= dataSize) {
            ESP_LOGW(TAG, "Command data truncated at command %d/%d", i, numCommands);
            break;
        }
        
        uint8_t cmdType = dataPtr[offset];
        size_t oldOffset = offset;
        offset++; // Skip command type byte
        
        // Execute command
        if (executeEnhancedCommand(cmdType, dataPtr, offset, xOffset, yOffset, map, dataSize))
 {
            executedCommands++;
            commandsExecuted++;
        } else {
            skippedCommands++;
            
            // Try to skip this command to continue with the next one
            if (offset == oldOffset + 1) {
                if (!skipUnknownCommand(cmdType, dataPtr, offset, dataSize)) {
                    ESP_LOGW(TAG, "Cannot recover from unknown command %d, stopping tile processing", cmdType);
                    break;
                }
            }
        }
        
        // Safety check to prevent infinite loops
        if (offset <= oldOffset) {
            ESP_LOGE(TAG, "Command %d didn't advance offset, stopping processing", cmdType);
            break;
        }
    }
    
    // Log results
    if (debugMode) {
        ESP_LOGI(TAG, "Tile %s complete: %d executed, %d skipped", 
                 path, executedCommands, skippedCommands);
    }
    
    tilesRendered++;
    return executedCommands > 0;
}


// Command execution dispatcher
// bool Maps::executeEnhancedCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, 
//                                  int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
//     try {
//         switch (cmdType) {
//             case DRAW_LINE:
//                 return drawLine(data, offset, xOffset, yOffset, map);
                
//             case DRAW_POLYLINE:
//                 return drawPolyline(data, offset, xOffset, yOffset, map);
                
//             case DRAW_FILL_RECT:
//                 return drawFillRect(data, offset, xOffset, yOffset, map);
                
//             case DRAW_FILL_POLYGON:
//                 return drawFillPolygon(data, offset, xOffset, yOffset, map);
                
//             case DRAW_STROKE_POLYGON:
//                 return drawStrokePolygon(data, offset, xOffset, yOffset, map);
                
//             case DRAW_HORIZONTAL_LINE:
//                 return drawHorizontalLine(data, offset, xOffset, yOffset, map);
                
//             case DRAW_VERTICAL_LINE:
//                 return drawVerticalLine(data, offset, xOffset, yOffset, map);
                
//             case DRAW_ADAPTIVE_LINE: {
//                 // Enhanced line with adaptive detail
//                 if (!checkBounds(offset, 5, map.width() * map.height())) return false;
                
//                 uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
//                 uint8_t complexity = *(uint8_t*)(data + offset); offset += sizeof(uint8_t);
//                 uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
                
//                 debugPrintCommand(cmdType, (GeometryComplexity)complexity);
                
//                 if (numPoints < 2 || numPoints > 2000) return false;
//                 if (!checkBounds(offset, numPoints * 4, map.width() * map.height())) return false;
                
//                 // Load points
//                 static std::vector<int16_t> points_x, points_y;
//                 points_x.clear(); points_y.clear();
//                 points_x.reserve(numPoints); points_y.reserve(numPoints);
                
//                 for (int p = 0; p < numPoints; ++p) {
//                     int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
//                     int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
//                     points_x.push_back(x);
//                     points_y.push_back(y);
//                 }
                
//                 drawAdaptiveLine(points_x.data(), points_y.data(), numPoints, color, 
//                                (GeometryComplexity)complexity, map);
//                 return true;
//             }
            
//             case DRAW_MULTI_LOD_POLYGON: {
//                 // Enhanced polygon with LOD support
//                 if (!checkBounds(offset, 6, map.width() * map.height())) return false;
                
//                 uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
//                 uint8_t complexity = *(uint8_t*)(data + offset); offset += sizeof(uint8_t);
//                 uint8_t render_style = *(uint8_t*)(data + offset); offset += sizeof(uint8_t);
//                 uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
                
//                 debugPrintCommand(cmdType, (GeometryComplexity)complexity);
                
//                 if (numPoints < 3 || numPoints > 1500) return false;
//                 if (!checkBounds(offset, numPoints * 4, map.width() * map.height())) return false;
                
//                 // Load points
//                 static std::vector<int16_t> points_x, points_y;
//                 points_x.clear(); points_y.clear();
//                 points_x.reserve(numPoints); points_y.reserve(numPoints);
                
//                 bool anyVisible = false;
//                 for (int p = 0; p < numPoints; ++p) {
//                     int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
//                     int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
//                     points_x.push_back(x);
//                     points_y.push_back(y);
                    
//                     if (x >= -200 && x < map.width() + 200 && y >= -200 && y < map.height() + 200) {
//                         anyVisible = true;
//                     }
//                 }
                
//                 if (anyVisible) {
//                     drawMultiLODPolygon(points_x.data(), points_y.data(), numPoints, color, 
//                                       (GeometryComplexity)complexity, render_style, map);
//                     return true;
//                 }
//                 return false;
//             }
            
//             case DRAW_SPLINE_CURVE: {
//                 // Smooth curve rendering
//                 if (!checkBounds(offset, 4, map.width() * map.height())) return false;
                
//                 uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
//                 uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
                
//                 debugPrintCommand(cmdType, COMPLEXITY_HIGH);
                
//                 if (numPoints < 3 || numPoints > 500) return false;
//                 if (!checkBounds(offset, numPoints * 4, map.width() * map.height())) return false;
                
//                 // Load control points
//                 static std::vector<int16_t> points_x, points_y;
//                 points_x.clear(); points_y.clear();
//                 points_x.reserve(numPoints); points_y.reserve(numPoints);
                
//                 for (int p = 0; p < numPoints; ++p) {
//                     int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
//                     int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
//                     points_x.push_back(x);
//                     points_y.push_back(y);
//                 }
                
//                 drawSmoothCurve(points_x.data(), points_y.data(), numPoints, color, map);
//                 return true;
//             }
            
//             default:
//                 ESP_LOGW(TAG, "Unknown enhanced command: %d", cmdType);
//                 return false;
//         }
//     } catch (...) {
//         ESP_LOGE(TAG, "Exception executing enhanced command %d", cmdType);
//         return false;
//     }
// }
// bool Maps::executeEnhancedCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, 
//                                  int16_t xOffset, int16_t yOffset, TFT_eSprite &map, size_t dataSize) {
//     try {
//         switch (cmdType) {
//             case DRAW_LINE: {
//                 // Ejemplo: leer 2 puntos + color (adaptar según definición real)
//                 // Supongo estructura: numPoints(2 bytes), color(2 bytes), puntos (x,y) cada 2 bytes
//                 if (!checkBounds(offset, 4 + 4, dataSize)) return false; // ejemplo mínimo
//                 // Lógica propia de drawLine debería estar aquí o en función aparte.
//                 // Aquí solo parseo segura.
//                 // Ajusta según tu formato real.

//                 // Ejemplo:
//                 uint16_t numPoints;
//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 uint16_t color;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 2 || numPoints > 2000) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawLine(points_x.data(), points_y.data(), numPoints, color, map);
//                 return true;
//             }

//             case DRAW_POLYLINE: {
//                 if (!checkBounds(offset, 4, dataSize)) return false;

//                 uint16_t numPoints;
//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 uint16_t color;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 2 || numPoints > 2000) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawPolyline(points_x.data(), points_y.data(), numPoints, color, map);
//                 return true;
//             }

//             case DRAW_FILL_RECT: {
//                 if (!checkBounds(offset, 10, dataSize)) return false;
//                 int16_t x, y;
//                 uint16_t w, h, color;

//                 memcpy(&x, data + offset, 2); offset += 2;
//                 memcpy(&y, data + offset, 2); offset += 2;
//                 memcpy(&w, data + offset, 2); offset += 2;
//                 memcpy(&h, data + offset, 2); offset += 2;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 x = clampCoord(x + xOffset);
//                 y = clampCoord(y + yOffset);

//                 drawFillRect(x, y, w, h, color, map);
//                 return true;
//             }

//             case DRAW_FILL_POLYGON: {
//                 if (!checkBounds(offset, 4, dataSize)) return false;
//                 uint16_t numPoints;
//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 uint16_t color;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 3 || numPoints > 1500) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawFillPolygon(points_x.data(), points_y.data(), numPoints, color, map);
//                 return true;
//             }

//             case DRAW_STROKE_POLYGON: {
//                 if (!checkBounds(offset, 4, dataSize)) return false;
//                 uint16_t numPoints;
//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 uint16_t color;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 3 || numPoints > 1500) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawStrokePolygon(points_x.data(), points_y.data(), numPoints, color, map);
//                 return true;
//             }

//             case DRAW_HORIZONTAL_LINE: {
//                 if (!checkBounds(offset, 6, dataSize)) return false;
//                 int16_t x, y;
//                 uint16_t length, color;

//                 memcpy(&x, data + offset, 2); offset += 2;
//                 memcpy(&y, data + offset, 2); offset += 2;
//                 memcpy(&length, data + offset, 2); offset += 2;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 drawHorizontalLine(x + xOffset, y + yOffset, length, color, map);
//                 return true;
//             }

//             case DRAW_VERTICAL_LINE: {
//                 if (!checkBounds(offset, 6, dataSize)) return false;
//                 int16_t x, y;
//                 uint16_t length, color;

//                 memcpy(&x, data + offset, 2); offset += 2;
//                 memcpy(&y, data + offset, 2); offset += 2;
//                 memcpy(&length, data + offset, 2); offset += 2;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 drawVerticalLine(x + xOffset, y + yOffset, length, color, map);
//                 return true;
//             }

//             case DRAW_ADAPTIVE_LINE: {
//                 if (!checkBounds(offset, 5, dataSize)) return false;

//                 uint16_t numPoints;
//                 uint8_t complexity;
//                 uint16_t color;

//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 memcpy(&complexity, data + offset, 1); offset += 1;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 2 || numPoints > 2000) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawAdaptiveLine(points_x.data(), points_y.data(), numPoints, color,
//                                 (GeometryComplexity)complexity, map);
//                 return true;
//             }

//             case DRAW_MULTI_LOD_POLYGON: {
//                 if (!checkBounds(offset, 6, dataSize)) return false;

//                 uint16_t numPoints;
//                 uint8_t complexity;
//                 uint8_t render_style;
//                 uint16_t color;

//                 memcpy(&numPoints, data + offset, 2); offset += 2;
//                 memcpy(&complexity, data + offset, 1); offset += 1;
//                 memcpy(&render_style, data + offset, 1); offset += 1;
//                 memcpy(&color, data + offset, 2); offset += 2;

//                 if (numPoints < 3 || numPoints > 1500) return false;
//                 if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

//                 std::vector<int16_t> points_x, points_y;
//                 points_x.reserve(numPoints);
//                 points_y.reserve(numPoints);

//                 for (uint16_t p = 0; p < numPoints; ++p) {
//                     int16_t x, y;
//                     memcpy(&x, data + offset, 2); offset += 2;
//                     memcpy(&y, data + offset, 2); offset += 2;
//                     points_x.push_back(clampCoord(x + xOffset));
//                     points_y.push_back(clampCoord(y + yOffset));
//                 }

//                 drawMultiLODPolygon(points_x.data(), points_y.data(), numPoints,
//                                     (GeometryComplexity)complexity,
//                                     (RenderStyle)render_style,
//                                     color, map);
//                 return true;
//             }

//             default:
//                 return false;
//         }
//     } catch (...) {
//         // Captura cualquier excepción (poco común en microcontroladores)
//         return false;
//     }
// }
bool Maps::executeEnhancedCommand(uint8_t cmdType, const uint8_t* data, size_t& offset,
                                  int16_t xOffset, int16_t yOffset, TFT_eSprite &map, size_t dataSize) {
    try {
        switch (cmdType) {
            case DRAW_LINE:
                return drawLine(data, offset, xOffset, yOffset, map);
            case DRAW_POLYLINE:
                return drawPolyline(data, offset, xOffset, yOffset, map);
            case DRAW_FILL_RECT:
                return drawFillRect(data, offset, xOffset, yOffset, map);
            case DRAW_FILL_POLYGON:
                return drawFillPolygon(data, offset, xOffset, yOffset, map);
            case DRAW_STROKE_POLYGON:
                return drawStrokePolygon(data, offset, xOffset, yOffset, map);
            case DRAW_HORIZONTAL_LINE:
                return drawHorizontalLine(data, offset, xOffset, yOffset, map);
            case DRAW_VERTICAL_LINE:
                return drawVerticalLine(data, offset, xOffset, yOffset, map);

            case DRAW_ADAPTIVE_LINE: {
                if (!checkBounds(offset, 5, dataSize)) return false;
                uint16_t numPoints = *(uint16_t*)(data + offset); offset += 2;
                uint8_t complexity = *(uint8_t*)(data + offset); offset += 1;
                uint16_t color = *(uint16_t*)(data + offset); offset += 2;

                debugPrintCommand(cmdType, (GeometryComplexity)complexity);
                if (numPoints < 2 || numPoints > 2000) return false;
                if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

                static std::vector<int16_t> points_x, points_y;
                points_x.clear(); points_y.clear();
                points_x.reserve(numPoints); points_y.reserve(numPoints);

                for (int p = 0; p < numPoints; ++p) {
                    int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += 2;
                    int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += 2;
                    points_x.push_back(x);
                    points_y.push_back(y);
                }

                drawAdaptiveLine(points_x.data(), points_y.data(), numPoints, color,
                                 (GeometryComplexity)complexity, map);
                return true;
            }

            case DRAW_MULTI_LOD_POLYGON: {
                if (!checkBounds(offset, 6, dataSize)) return false;
                uint16_t numPoints = *(uint16_t*)(data + offset); offset += 2;
                uint8_t complexity = *(uint8_t*)(data + offset); offset += 1;
                uint8_t render_style = *(uint8_t*)(data + offset); offset += 1;
                uint16_t color = *(uint16_t*)(data + offset); offset += 2;

                debugPrintCommand(cmdType, (GeometryComplexity)complexity);
                if (numPoints < 3 || numPoints > 1500) return false;
                if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

                static std::vector<int16_t> points_x, points_y;
                points_x.clear(); points_y.clear();
                points_x.reserve(numPoints); points_y.reserve(numPoints);

                bool anyVisible = false;
                for (int p = 0; p < numPoints; ++p) {
                    int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += 2;
                    int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += 2;
                    points_x.push_back(x);
                    points_y.push_back(y);

                    if (x >= -200 && x < map.width() + 200 && y >= -200 && y < map.height() + 200)
                        anyVisible = true;
                }

                if (anyVisible) {
                    drawMultiLODPolygon(points_x.data(), points_y.data(), numPoints, color,
                                        (GeometryComplexity)complexity, render_style, map);
                    return true;
                }
                return false;
            }

            case DRAW_SPLINE_CURVE: {
                if (!checkBounds(offset, 4, dataSize)) return false;
                uint16_t numPoints = *(uint16_t*)(data + offset); offset += 2;
                uint16_t color = *(uint16_t*)(data + offset); offset += 2;

                debugPrintCommand(cmdType, COMPLEXITY_HIGH);
                if (numPoints < 3 || numPoints > 500) return false;
                if (!checkBounds(offset, numPoints * 4, dataSize)) return false;

                static std::vector<int16_t> points_x, points_y;
                points_x.clear(); points_y.clear();
                points_x.reserve(numPoints); points_y.reserve(numPoints);

                for (int p = 0; p < numPoints; ++p) {
                    int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += 2;
                    int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += 2;
                    points_x.push_back(x);
                    points_y.push_back(y);
                }

                drawSmoothCurve(points_x.data(), points_y.data(), numPoints, color, map);
                return true;
            }

            default:
                ESP_LOGW(TAG, "Unknown enhanced command: %d", cmdType);
                return false;
        }
    } catch (...) {
        ESP_LOGE(TAG, "Exception executing enhanced command %d", cmdType);
        return false;
    }
}



// Basic drawing command implementations
bool Maps::drawLine(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 10, MAX_TILE_SIZE_BYTES)) return false;
    
    int16_t x1 = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t y1 = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    int16_t x2 = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t y2 = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_LINE, COMPLEXITY_LOW);
    
            if (isVisible(std::min(x1, x2), std::min(y1, y2), std::abs(x2-x1), std::abs(y2-y1), map)) {
        map.drawLine(x1, y1, x2, y2, color);
    }
	
    
    return true;
}

bool Maps::drawPolyline(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 4, MAX_TILE_SIZE_BYTES)) return false;
    
    uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_POLYLINE, COMPLEXITY_MEDIUM);
    
    if (numPoints < 2 || numPoints > 3000) return false;
    if (!checkBounds(offset, numPoints * 4, MAX_TILE_SIZE_BYTES)) return false;
    
    // Draw lines between consecutive points
    int16_t prevX = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t prevY = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    
    for (int p = 1; p < numPoints; ++p) {
        int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
        int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
        
        if (isVisible(std::min(prevX, x), std::min(prevY, y), std::abs(x-prevX), std::abs(y-prevY), map)) {
            map.drawLine(prevX, prevY, x, y, color);
        }
        
        prevX = x;
        prevY = y;
    }
    
    return true;
}

bool Maps::drawFillRect(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 10, MAX_TILE_SIZE_BYTES)) return false;
    
    int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    int16_t w = *(int16_t*)(data + offset); offset += sizeof(int16_t);
    int16_t h = *(int16_t*)(data + offset); offset += sizeof(int16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_FILL_RECT, COMPLEXITY_LOW);
    
    if (w > 0 && h > 0 && isVisible(x, y, w, h, map)) {
        map.fillRect(x, y, w, h, color);
    }
    
    return true;
}

bool Maps::drawFillPolygon(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 4, MAX_TILE_SIZE_BYTES)) return false;
    
    uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_FILL_POLYGON, COMPLEXITY_HIGH);
    
    if (numPoints < 3 || numPoints > 3000) return false;
    if (!checkBounds(offset, numPoints * 4, MAX_TILE_SIZE_BYTES)) return false;
    
    // Load points
    static std::vector<int16_t> points_x, points_y;
    points_x.clear(); points_y.clear();
    points_x.reserve(numPoints); points_y.reserve(numPoints);
    
    bool anyVisible = false;
    for (int p = 0; p < numPoints; ++p) {
        int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
        int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
        points_x.push_back(x);
        points_y.push_back(y);
        
        if (x >= -200 && x < map.width() + 200 && y >= -200 && y < map.height() + 200) {
            anyVisible = true;
        }
    }
    
    if (anyVisible) {
        drawPolygonFast(points_x.data(), points_y.data(), numPoints, color, map);
    }
    
    return true;
}

bool Maps::drawStrokePolygon(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 4, MAX_TILE_SIZE_BYTES)) return false;
    
    uint16_t numPoints = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_STROKE_POLYGON, COMPLEXITY_MEDIUM);
    
    if (numPoints < 3 || numPoints > 3000) return false;
    if (!checkBounds(offset, numPoints * 4, MAX_TILE_SIZE_BYTES)) return false;
    
    // Draw polygon outline
    int16_t firstX = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t firstY = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    int16_t prevX = firstX, prevY = firstY;
    
    for (int p = 1; p < numPoints; ++p) {
        int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
        int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
        
        if (isVisible(std::min(prevX, x), std::min(prevY, y), std::abs(x-prevX), std::abs(y-prevY), map)) {
            map.drawLine(prevX, prevY, x, y, color);
        }
        
        prevX = x; prevY = y;
    }
    
    // Close the polygon
    if (isVisible(std::min(prevX, firstX), std::min(prevY, firstY), std::abs(firstX-prevX), std::abs(firstY-prevY), map)) {
        map.drawLine(prevX, prevY, firstX, firstY, color);
    }
    
    return true;
}

bool Maps::drawHorizontalLine(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 8, MAX_TILE_SIZE_BYTES)) return false;
    
    int16_t x1 = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t x2 = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t y = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_HORIZONTAL_LINE, COMPLEXITY_LOW);
    
    if (x1 > x2) std::swap(x1, x2);
    
    if (isVisible(x1, y, x2-x1, 1, map)) {
        map.drawFastHLine(x1, y, x2-x1+1, color);
    }
    
    return true;
}

bool Maps::drawVerticalLine(const uint8_t* data, size_t& offset, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) {
    if (!checkBounds(offset, 8, MAX_TILE_SIZE_BYTES)) return false;
    
    int16_t x = clampCoord(*(int16_t*)(data + offset) + xOffset); offset += sizeof(int16_t);
    int16_t y1 = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    int16_t y2 = clampCoord(*(int16_t*)(data + offset) + yOffset); offset += sizeof(int16_t);
    uint16_t color = *(uint16_t*)(data + offset); offset += sizeof(uint16_t);
    
    debugPrintCommand(DRAW_VERTICAL_LINE, COMPLEXITY_LOW);
    
    if (y1 > y2) std::swap(y1, y2);
    
    if (isVisible(x, y1, 1, y2-y1, map)) {
        map.drawFastVLine(x, y1, y2-y1+1, color);
    }
    
    return true;
}

// Enhanced drawing implementations
void Maps::drawAdaptiveLine(const int16_t* points_x, const int16_t* points_y, 
                           size_t count, uint16_t color, GeometryComplexity complexity, TFT_eSprite &map) {
    if (count < 2) return;
    
    // Adaptive rendering based on complexity and quality level
    if (complexity == COMPLEXITY_HIGH && qualityLevel >= 2) {
        // High quality: interpolate additional points for smooth curves
        static std::vector<int16_t> smooth_x, smooth_y;
        interpolatePoints(points_x, points_y, count, smooth_x, smooth_y);
        
        for (size_t i = 1; i < smooth_x.size(); ++i) {
            if (isVisible(std::min(smooth_x[i-1], smooth_x[i]), std::min(smooth_y[i-1], smooth_y[i]), 
                         std::abs(smooth_x[i]-smooth_x[i-1]), std::abs(smooth_y[i]-smooth_y[i-1]), map)) {
                map.drawLine(smooth_x[i-1], smooth_y[i-1], smooth_x[i], smooth_y[i], color);
            }
        }
    } else {
        // Standard quality: direct line drawing
        for (size_t i = 1; i < count; ++i) {
            if (isVisible(std::min(points_x[i-1], points_x[i]), std::min(points_y[i-1], points_y[i]), 
                         std::abs(points_x[i]-points_x[i-1]), std::abs(points_y[i]-points_y[i-1]), map)) {
                map.drawLine(points_x[i-1], points_y[i-1], points_x[i], points_y[i], color);
            }
        }
    }
}

void Maps::drawMultiLODPolygon(const int16_t* points_x, const int16_t* points_y, 
                              size_t count, uint16_t color, GeometryComplexity complexity, 
                              uint8_t render_style, TFT_eSprite &map) {
    if (count < 3) return;
    
    // Quality-based LOD selection
    size_t effective_points = count;
    
    if (qualityLevel == 1 && complexity == COMPLEXITY_HIGH && count > 100) {
        // Fast mode: reduce points for complex polygons
        effective_points = count / 2;
    }
    
    if (render_style == 0) {  // Stroke only
        for (size_t i = 1; i < effective_points; ++i) {
            map.drawLine(points_x[i-1], points_y[i-1], points_x[i], points_y[i], color);
        }
        // Close polygon
        map.drawLine(points_x[effective_points-1], points_y[effective_points-1], points_x[0], points_y[0], color);
    } else {  // Filled
        if (effective_points < count) {
            // Create reduced point array
            static std::vector<int16_t> reduced_x, reduced_y;
            reduced_x.clear(); reduced_y.clear();
            reduced_x.reserve(effective_points);
            reduced_y.reserve(effective_points);
            
            for (size_t i = 0; i < effective_points; ++i) {
                size_t idx = i * count / effective_points;
                if (idx >= count) idx = count - 1;
                reduced_x.push_back(points_x[idx]);
                reduced_y.push_back(points_y[idx]);
            }
            
            drawPolygonFast(reduced_x.data(), reduced_y.data(), effective_points, color, map);
        } else {
            drawPolygonFast(points_x, points_y, count, color, map);
        }
    }
}

void Maps::drawSmoothCurve(const int16_t* points_x, const int16_t* points_y, 
                          size_t count, uint16_t color, TFT_eSprite &map) {
    if (count < 3) return;
    
    // Generate smooth curve using Catmull-Rom spline
    static std::vector<int16_t> curve_x, curve_y;
    catmullRomSpline(points_x, points_y, count, curve_x, curve_y);
    
    // Draw the smooth curve
    for (size_t i = 1; i < curve_x.size(); ++i) {
        if (isVisible(std::min(curve_x[i-1], curve_x[i]), std::min(curve_y[i-1], curve_y[i]), 
                     std::abs(curve_x[i]-curve_x[i-1]), std::abs(curve_y[i]-curve_y[i-1]), map)) {
            map.drawLine(curve_x[i-1], curve_y[i-1], curve_x[i], curve_y[i], color);
        }
    }
}

// Fast polygon filling using scanline algorithm
void Maps::drawPolygonFast(const int16_t* points_x, const int16_t* points_y, 
                          size_t count, uint16_t color, TFT_eSprite &map) {
    if (count < 3) return;
    
    // Find bounds
    int16_t min_y = points_y[0], max_y = points_y[0];
    for (size_t i = 1; i < count; ++i) {
        if (points_y[i] < min_y) min_y = points_y[i];
        if (points_y[i] > max_y) max_y = points_y[i];
    }
    
    // Clip to screen bounds
    min_y = std::max((int16_t)0, min_y);
    max_y = std::min((int16_t)(map.height() - 1), max_y);
    
    // Scanline algorithm
    for (int16_t y = min_y; y <= max_y; ++y) {
        std::vector<int16_t> intersections;
        intersections.reserve(count);
        
        // Find intersections with polygon edges
        for (size_t i = 0; i < count; ++i) {
            size_t j = (i + 1) % count;
            
            int16_t y1 = points_y[i], y2 = points_y[j];
            if (y1 > y2) { std::swap(y1, y2); }
            
            if (y >= y1 && y < y2 && y1 != y2) {
                int16_t x1 = points_x[i], x2 = points_x[j];
                int16_t x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                intersections.push_back(x);
            }
        }
        
        // Sort intersections and fill between pairs
        std::sort(intersections.begin(), intersections.end());
        
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int16_t x1 = std::max((int16_t)0, intersections[i]);
            int16_t x2 = std::min((int16_t)(map.width() - 1), intersections[i + 1]);
            
            if (x1 <= x2) {
                map.drawFastHLine(x1, y, x2 - x1 + 1, color);
            }
        }
    }
}

// Interpolation functions
void Maps::interpolatePoints(const int16_t* points_x, const int16_t* points_y, 
                            size_t count, std::vector<int16_t>& out_x, std::vector<int16_t>& out_y) {
    out_x.clear();
    out_y.clear();
    
    if (count < 2) return;
    
    // Reserve space for efficiency
    out_x.reserve(count * 2);
    out_y.reserve(count * 2);
    
    // Simple linear interpolation
    for (size_t i = 0; i < count - 1; ++i) {
        out_x.push_back(points_x[i]);
        out_y.push_back(points_y[i]);
        
        // Add interpolated point if segment is long enough
        int16_t dx = points_x[i+1] - points_x[i];
        int16_t dy = points_y[i+1] - points_y[i];
        int32_t dist_sq = dx * dx + dy * dy;
        
        if (dist_sq > 100) {  // If distance > 10 pixels
            out_x.push_back(points_x[i] + dx/2);
            out_y.push_back(points_y[i] + dy/2);
        }
    }
    
    // Add last point
    out_x.push_back(points_x[count-1]);
    out_y.push_back(points_y[count-1]);
}

void Maps::catmullRomSpline(const int16_t* points_x, const int16_t* points_y, 
                           size_t count, std::vector<int16_t>& out_x, std::vector<int16_t>& out_y) {
    out_x.clear();
    out_y.clear();
    
    if (count < 4) {
        // Fallback to linear interpolation
        interpolatePoints(points_x, points_y, count, out_x, out_y);
        return;
    }
    
    // Reserve space
    size_t estimatedPoints = (count - 3) * 10 + count;
    out_x.reserve(estimatedPoints);
    out_y.reserve(estimatedPoints);
    
    // Catmull-Rom spline implementation
    for (size_t i = 1; i < count - 2; ++i) {
        // Control points
        float p0x = points_x[i-1], p0y = points_y[i-1];
        float p1x = points_x[i], p1y = points_y[i];
        float p2x = points_x[i+1], p2y = points_y[i+1];
        float p3x = points_x[i+2], p3y = points_y[i+2];
        
        // Generate curve points (quality-dependent resolution)
        int steps = (qualityLevel == 1) ? 5 : (qualityLevel == 2) ? 8 : 12;
        
        for (int j = 0; j < steps; ++j) {
            float t = (float)j / steps;
            float t2 = t * t;
            float t3 = t2 * t;
            
            // Catmull-Rom formula
            float x = 0.5f * ((2.0f * p1x) +
                            (-p0x + p2x) * t +
                            (2.0f * p0x - 5.0f * p1x + 4.0f * p2x - p3x) * t2 +
                            (-p0x + 3.0f * p1x - 3.0f * p2x + p3x) * t3);
            
            float y = 0.5f * ((2.0f * p1y) +
                            (-p0y + p2y) * t +
                            (2.0f * p0y - 5.0f * p1y + 4.0f * p2y - p3y) * t2 +
                            (-p0y + 3.0f * p1y - 3.0f * p2y + p3y) * t3);
            
            // Clamp coordinates
            x = std::max(-32767.0f, std::min(32767.0f, x));
            y = std::max(-32767.0f, std::min(32767.0f, y));
            
            out_x.push_back((int16_t)x);
            out_y.push_back((int16_t)y);
        }
    }
}

// Recovery mechanism for unknown commands
bool Maps::skipUnknownCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, size_t dataSize) {
    // Try to skip based on known command patterns
    switch (cmdType) {
        case DRAW_LINE:
        case DRAW_FILL_RECT:
            // 4 int16 + 1 uint16 = 10 bytes
            if (offset + 10 <= dataSize) {
                offset += 10;
                return true;
            }
            break;
            
        case DRAW_HORIZONTAL_LINE:
        case DRAW_VERTICAL_LINE:
            // 3 int16 + 1 uint16 = 8 bytes
            if (offset + 8 <= dataSize) {
                offset += 8;
                return true;
            }
            break;
            
        case DRAW_POLYLINE:
        case DRAW_FILL_POLYGON:
        case DRAW_STROKE_POLYGON:
        case DRAW_SPLINE_CURVE:
            // Variable length: num_points(2) + color(2) + points(num_points * 4)
            if (offset + 4 <= dataSize) {
                uint16_t numPoints = *(uint16_t*)(data + offset);
                size_t totalSize = 4 + numPoints * 4;
                if (offset + totalSize <= dataSize && numPoints <= 5000) {
                    offset += totalSize;
                    return true;
                }
            }
            break;
            
        default:
            // Unknown command - try minimal skip
            ESP_LOGW(TAG, "Unknown command type %d, attempting minimal skip", cmdType);
            if (offset + 2 <= dataSize) {
                offset += 2;
                return true;
            }
            break;
    }
    
    return false;
}