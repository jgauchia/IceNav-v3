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

uint16_t Maps::currentDrawColor = TFT_WHITE;
bool Maps::fillPolygons = false;  // Default: solo contorno como Python

/**
 * @brief Map Class constructor
 */
Maps::Maps() {}

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
		const int16_t size = Maps::mapTileSize;

        Maps::mapTempSprite.fillScreen(TFT_BLACK);

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


///*********************************************************** */

/**
 * @brief Read variable-length integer (varint) from binary data.
 */
static uint32_t read_varint(const uint8_t* data, size_t& offset, size_t dataSize)
{
    uint32_t value = 0;
    uint8_t shift = 0;
    
    while (offset < dataSize && shift < 32) {
        uint8_t byte = data[offset++];
        value |= ((uint32_t)(byte & 0x7F)) << shift;
        
        if ((byte & 0x80) == 0) {
            break;
        }
        
        shift += 7;
    }
    
    return value;
}

/**
 * @brief Read zigzag-encoded signed integer from binary data - FORMATO SCRIPT PYTHON
 */
static int32_t read_zigzag(const uint8_t* data, size_t& offset, size_t dataSize)
{
    uint32_t encoded = read_varint(data, offset, dataSize);
    return (int32_t)((encoded >> 1) ^ (-(int32_t)(encoded & 1)));
}

/**
 * @brief Convert uint16 coordinate to tile pixel (0-255) - CORREGIDO PARA COORDENADAS 0-255
 */
static int uint16_to_tile_pixel(int32_t val) {
    return (int)((val * 256) / 65536);
}

/**
 * @brief Swap bytes in RGB565 color for display compatibility
 */
static uint16_t swapRGB565Bytes(uint16_t color)
{
    return ((color & 0xFF) << 8) | ((color & 0xFF00) >> 8);
}

/**
 * @brief Convert RGB332 to RGB565 - CON SWAP DE BYTES PARA PANTALLA
 */
static uint16_t rgb332_to_rgb565(uint8_t c)
{
    // Conversión EXACTA del script PC: rgb332_to_rgb888()
    // r = (c & 0xE0)        # Mantiene bits 7-5, bits 4-0 = 0
    // g = (c & 0x1C) << 3   # Toma bits 4-2, shift left 3 → bits 7-5  
    // b = (c & 0x03) << 6   # Toma bits 1-0, shift left 6 → bits 7-6
    
    uint8_t r8 = (c & 0xE0);        // Red: bits 7-5, resto 0 → 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0
    uint8_t g8 = (c & 0x1C) << 3;   // Green: bits 4-2 → 7-5 → 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0  
    uint8_t b8 = (c & 0x03) << 6;   // Blue: bits 1-0 → 7-6 → 0x00, 0x40, 0x80, 0xC0
    
    // Convertir de RGB888 a RGB565
    // RGB565: RRRRRGGGGGGBBBBB
    uint16_t r5 = (r8 >> 3);  // 8 bits → 5 bits (tomar bits 7-3)
    uint16_t g6 = (g8 >> 2);  // 8 bits → 6 bits (tomar bits 7-2)
    uint16_t b5 = (b8 >> 3);  // 8 bits → 5 bits (tomar bits 7-3)
    
    // Ensamblar RGB565
    uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
    
    // SWAP BYTES PARA PANTALLA
    return swapRGB565Bytes(rgb565);
}

/**
 * @brief Check if point is on tile margin (0 or 255) - FILTRO COMO TILE_VIEWER.PY
 */
static bool isPointOnMargin(int px, int py)
{
    return (px == 0 || px == 255 || py == 0 || py == 255);
}

/**
 * @brief Check if line should be drawn (not on margins) - FILTRO COMO TILE_VIEWER.PY
 */
static bool shouldDrawLine(int px1, int py1, int px2, int py2)
{
    // NO DIBUJAR COMPLETAMENTE si algún punto está en los márgenes del tile
    return !isPointOnMargin(px1, py1) && !isPointOnMargin(px2, py2);
}

/**
 * @brief Renders a vector tile with ALL possible drawing commands - CON SWAP DE BYTES PARA PANTALLA
 */
bool Maps::renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map)
{
    if (!path) {
        return false;
    }
    
    // 1. LEER ARCHIVO COMPLETO
    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > 100000) {
        fclose(file);
        return false;
    }
    
    uint8_t* data = new uint8_t[fileSize];
    if (!data) {
        fclose(file);
        return false;
    }
    
    size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    
    if (bytesRead != fileSize) {
        delete[] data;
        return false;
    }
    
    // 2. PROCESAR FORMATO SCRIPT PYTHON
    size_t offset = 0;
    size_t dataSize = fileSize;
    uint8_t current_color = 0xFF; // Color RGB332 por defecto (blanco)
    uint16_t currentDrawColor = rgb332_to_rgb565(current_color); // Color actual para dibujar
    
    // Leer número de comandos
    uint32_t num_cmds = read_varint(data, offset, dataSize);
    
    if (num_cmds == 0 || num_cmds > 50000) {
        delete[] data;
        return false;
    }
    
    // 3. PROCESAR TODOS LOS COMANDOS POSIBLES - CON SWAP DE BYTES PARA PANTALLA
    int executed = 0;
    
    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) {
        if (offset >= dataSize) {
            break;
        }
        
        size_t cmd_start_offset = offset;
        uint32_t cmd_type = read_varint(data, offset, dataSize);
        
        // PROCESAR TODOS LOS COMANDOS POSIBLES
        switch (cmd_type) {
            case 0x80: // SET_COLOR - CON SWAP DE BYTES
                if (offset < dataSize) {
                    current_color = data[offset++];
                    currentDrawColor = rgb332_to_rgb565(current_color);
                    executed++;
                }
                break;
                
            case 0x81: // SET_COLOR_INDEX - CON SWAP DE BYTES
                {
                    uint32_t color_index = read_varint(data, offset, dataSize);
                    current_color = color_index & 0xFF;
                    currentDrawColor = rgb332_to_rgb565(current_color);
                    executed++;
                }
                break;
                
            case 1: // LINE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    
                    int32_t x2 = x1 + dx;
                    int32_t y2 = y1 + dy;
                    
                    // Convertir a píxeles
                    int px1 = uint16_to_tile_pixel(x1);
                    int py1 = uint16_to_tile_pixel(y1);
                    int px2 = uint16_to_tile_pixel(x2);
                    int py2 = uint16_to_tile_pixel(y2);
                    
                    // SOLO dibujar si NO toca márgenes Y está en bounds
                    if (px1 >= 0 && px1 <= 255 && py1 >= 0 && py1 <= 255 &&
                        px2 >= 0 && px2 <= 255 && py2 >= 0 && py2 <= 255 &&
                        shouldDrawLine(px1, py1, px2, py2)) {
                        map.drawLine(px1 + xOffset, py1 + yOffset, px2 + xOffset, py2 + yOffset, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada (no dibujar línea negra)
                }
                break;
                
            case 2: // POLYLINE - NO DIBUJAR SEGMENTOS QUE TOCAN MÁRGENES
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    
                    if (num_points >= 2 && num_points <= 1000) {
                        // Primer punto (absoluto)
                        int32_t prevX = read_zigzag(data, offset, dataSize);
                        int32_t prevY = read_zigzag(data, offset, dataSize);
                        
                        int prevPx = uint16_to_tile_pixel(prevX);
                        int prevPy = uint16_to_tile_pixel(prevY);
                        
                        // Puntos siguientes (relativos)
                        for (uint32_t i = 1; i < num_points; ++i) {
                            int32_t deltaX = read_zigzag(data, offset, dataSize);
                            int32_t deltaY = read_zigzag(data, offset, dataSize);
                            
                            prevX += deltaX;
                            prevY += deltaY;
                            
                            int currentPx = uint16_to_tile_pixel(prevX);
                            int currentPy = uint16_to_tile_pixel(prevY);
                            
                            // SOLO dibujar segmento si NO toca márgenes
                            if (prevPx >= 0 && prevPx <= 255 && prevPy >= 0 && prevPy <= 255 &&
                                currentPx >= 0 && currentPx <= 255 && currentPy >= 0 && currentPy <= 255 &&
                                shouldDrawLine(prevPx, prevPy, currentPx, currentPy)) {
                                map.drawLine(prevPx + xOffset, prevPy + yOffset, 
                                            currentPx + xOffset, currentPy + yOffset, currentDrawColor);
                            }
                            // Si toca márgenes: NO hacer nada (saltar segmento)
                            
                            prevPx = currentPx;
                            prevPy = currentPy;
                        }
                        executed++;
                    } else {
                        // Skip invalid polyline
                        for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                            if (i == 0) {
                                read_zigzag(data, offset, dataSize);
                                read_zigzag(data, offset, dataSize);
                            } else {
                                read_zigzag(data, offset, dataSize);
                                read_zigzag(data, offset, dataSize);
                            }
                        }
                    }
                }
                break;
                
            case 3: // STROKE_POLYGON - NO DIBUJAR SEGMENTOS QUE TOCAN MÁRGENES
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    
                    if (num_points >= 3 && num_points <= 1000) {
                        // Primer punto (absoluto)
                        int32_t firstX = read_zigzag(data, offset, dataSize);
                        int32_t firstY = read_zigzag(data, offset, dataSize);
                        
                        int firstPx = uint16_to_tile_pixel(firstX);
                        int firstPy = uint16_to_tile_pixel(firstY);
                        
                        int prevPx = firstPx;
                        int prevPy = firstPy;
                        
                        // Puntos siguientes (relativos)
                        for (uint32_t i = 1; i < num_points; ++i) {
                            int32_t deltaX = read_zigzag(data, offset, dataSize);
                            int32_t deltaY = read_zigzag(data, offset, dataSize);
                            
                            firstX += deltaX;
                            firstY += deltaY;
                            
                            int currentPx = uint16_to_tile_pixel(firstX);
                            int currentPy = uint16_to_tile_pixel(firstY);
                            
                            // SOLO dibujar segmento si NO toca márgenes
                            if (prevPx >= 0 && prevPx <= 255 && prevPy >= 0 && prevPy <= 255 &&
                                currentPx >= 0 && currentPx <= 255 && currentPy >= 0 && currentPy <= 255 &&
                                shouldDrawLine(prevPx, prevPy, currentPx, currentPy)) {
                                map.drawLine(prevPx + xOffset, prevPy + yOffset, 
                                            currentPx + xOffset, currentPy + yOffset, currentDrawColor);
                            }
                            // Si toca márgenes: NO hacer nada (saltar segmento)
                            
                            prevPx = currentPx;
                            prevPy = currentPy;
                        }
                        
                        // Cerrar polígono SOLO si NO toca márgenes
                        if (prevPx >= 0 && prevPx <= 255 && prevPy >= 0 && prevPy <= 255 &&
                            firstPx >= 0 && firstPx <= 255 && firstPy >= 0 && firstPy <= 255 &&
                            shouldDrawLine(prevPx, prevPy, firstPx, firstPy)) {
                            map.drawLine(prevPx + xOffset, prevPy + yOffset, 
                                        firstPx + xOffset, firstPy + yOffset, currentDrawColor);
                        }
                        // Si toca márgenes: NO hacer nada (no cerrar polígono)
                        executed++;
                    } else {
                        // Skip invalid polygon
                        for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                            if (i == 0) {
                                read_zigzag(data, offset, dataSize);
                                read_zigzag(data, offset, dataSize);
                            } else {
                                read_zigzag(data, offset, dataSize);
                                read_zigzag(data, offset, dataSize);
                            }
                        }
                    }
                }
                break;
                
            case 4: // FILL_POLYGON
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    
                    // Skip - implementación compleja de relleno
                    int32_t accumX = 0, accumY = 0;
                    for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                        if (i == 0) {
                            accumX = read_zigzag(data, offset, dataSize);
                            accumY = read_zigzag(data, offset, dataSize);
                        } else {
                            int32_t deltaX = read_zigzag(data, offset, dataSize);
                            int32_t deltaY = read_zigzag(data, offset, dataSize);
                            accumX += deltaX;
                            accumY += deltaY;
                        }
                    }
                }
                break;
                
            case 5: // HORIZONTAL_LINE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t y = read_zigzag(data, offset, dataSize);
                    
                    int32_t x2 = x1 + dx;
                    
                    int px1 = uint16_to_tile_pixel(x1);
                    int px2 = uint16_to_tile_pixel(x2);
                    int py = uint16_to_tile_pixel(y);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (px1 >= 0 && px1 <= 255 && px2 >= 0 && px2 <= 255 && py >= 0 && py <= 255 &&
                        shouldDrawLine(px1, py, px2, py)) {
                        map.drawLine(px1 + xOffset, py + yOffset, px2 + xOffset, py + yOffset, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 6: // VERTICAL_LINE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    
                    int32_t y2 = y1 + dy;
                    
                    int px = uint16_to_tile_pixel(x);
                    int py1 = uint16_to_tile_pixel(y1);
                    int py2 = uint16_to_tile_pixel(y2);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (px >= 0 && px <= 255 && py1 >= 0 && py1 <= 255 && py2 >= 0 && py2 <= 255 &&
                        shouldDrawLine(px, py1, px, py2)) {
                        map.drawLine(px + xOffset, py1 + yOffset, px + xOffset, py2 + yOffset, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x82: // RECTANGLE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    
                    int px1 = uint16_to_tile_pixel(x1);
                    int py1 = uint16_to_tile_pixel(y1);
                    int pwidth = uint16_to_tile_pixel(dx);
                    int pheight = uint16_to_tile_pixel(dy);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (px1 >= 0 && px1 <= 255 && py1 >= 0 && py1 <= 255 && 
                        pwidth > 0 && pheight > 0 &&
                        !isPointOnMargin(px1, py1) && 
                        !isPointOnMargin(px1 + pwidth, py1 + pheight)) {
                        if (fillPolygons) {
                            map.fillRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                        } else {
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                        }
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x83: // STRAIGHT_LINE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    
                    int32_t x2 = x1 + dx;
                    int32_t y2 = y1 + dy;
                    
                    int px1 = uint16_to_tile_pixel(x1);
                    int py1 = uint16_to_tile_pixel(y1);
                    int px2 = uint16_to_tile_pixel(x2);
                    int py2 = uint16_to_tile_pixel(y2);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (px1 >= 0 && px1 <= 255 && py1 >= 0 && py1 <= 255 &&
                        px2 >= 0 && px2 <= 255 && py2 >= 0 && py2 <= 255 &&
                        shouldDrawLine(px1, py1, px2, py2)) {
                        map.drawLine(px1 + xOffset, py1 + yOffset, px2 + xOffset, py2 + yOffset, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x84: // ELLIPSE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t center_x = read_zigzag(data, offset, dataSize);
                    int32_t center_y = read_zigzag(data, offset, dataSize);
                    int32_t radius_x = read_zigzag(data, offset, dataSize);
                    int32_t radius_y = read_zigzag(data, offset, dataSize);
                    
                    int pcx = uint16_to_tile_pixel(center_x);
                    int pcy = uint16_to_tile_pixel(center_y);
                    int pradius = uint16_to_tile_pixel((radius_x + radius_y) / 2);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (pcx >= 0 && pcx <= 255 && pcy >= 0 && pcy <= 255 && pradius > 0 &&
                        !isPointOnMargin(pcx, pcy)) {
                        if (fillPolygons) {
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        } else {
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        }
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x85: // ARC - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t center_x = read_zigzag(data, offset, dataSize);
                    int32_t center_y = read_zigzag(data, offset, dataSize);
                    int32_t radius = read_zigzag(data, offset, dataSize);
                    int32_t start_angle = read_zigzag(data, offset, dataSize);
                    int32_t end_angle = read_zigzag(data, offset, dataSize);
                    
                    int pcx = uint16_to_tile_pixel(center_x);
                    int pcy = uint16_to_tile_pixel(center_y);
                    int pradius = uint16_to_tile_pixel(radius);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (pcx >= 0 && pcx <= 255 && pcy >= 0 && pcy <= 255 && pradius > 0 &&
                        !isPointOnMargin(pcx, pcy)) {
                        map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x86: // BEZIER_CURVE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t x2 = read_zigzag(data, offset, dataSize);
                    int32_t y2 = read_zigzag(data, offset, dataSize);
                    int32_t x3 = read_zigzag(data, offset, dataSize);
                    int32_t y3 = read_zigzag(data, offset, dataSize);
                    int32_t x4 = read_zigzag(data, offset, dataSize);
                    int32_t y4 = read_zigzag(data, offset, dataSize);
                    
                    // Simplificar como línea recta - NO DIBUJAR SI TOCA MÁRGENES
                    int px1 = uint16_to_tile_pixel(x1);
                    int py1 = uint16_to_tile_pixel(y1);
                    int px4 = uint16_to_tile_pixel(x4);
                    int py4 = uint16_to_tile_pixel(y4);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (px1 >= 0 && px1 <= 255 && py1 >= 0 && py1 <= 255 &&
                        px4 >= 0 && px4 <= 255 && py4 >= 0 && py4 <= 255 &&
                        shouldDrawLine(px1, py1, px4, py4)) {
                        map.drawLine(px1 + xOffset, py1 + yOffset, px4 + xOffset, py4 + yOffset, currentDrawColor);
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x87: // CIRCLE - NO DIBUJAR SI TOCA MÁRGENES
                {
                    int32_t center_x = read_zigzag(data, offset, dataSize);
                    int32_t center_y = read_zigzag(data, offset, dataSize);
                    int32_t radius = read_zigzag(data, offset, dataSize);
                    
                    int pcx = uint16_to_tile_pixel(center_x);
                    int pcy = uint16_to_tile_pixel(center_y);
                    int pradius = uint16_to_tile_pixel(radius);
                    
                    // SOLO dibujar si NO toca márgenes
                    if (pcx >= 0 && pcx <= 255 && pcy >= 0 && pcy <= 255 && pradius > 0 &&
                        !isPointOnMargin(pcx, pcy)) {
                        if (fillPolygons) {
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        } else {
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        }
                        executed++;
                    }
                    // Si toca márgenes: NO hacer nada
                }
                break;
                
            case 0x88: // TEXT
                {
                    int32_t x = read_zigzag(data, offset, dataSize);
                    int32_t y = read_zigzag(data, offset, dataSize);
                    uint32_t text_len = read_varint(data, offset, dataSize);
                    
                    if (text_len > 0 && text_len < 100 && offset + text_len <= dataSize) {
                        offset += text_len; // Skip text content for now
                        executed++;
                    } else {
                        offset += std::min(text_len, (uint32_t)(dataSize - offset));
                    }
                }
                break;
                
            default:
                // Skip unknown command
                if (offset < dataSize - 4) {
                    offset += 4;
                }
                break;
        }
        
        // Verificar progreso para evitar loops infinitos
        if (offset <= cmd_start_offset) {
            break;
        }
    }
    
    delete[] data;
    return executed > 0;
}