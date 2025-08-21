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

        Maps::mapTempSprite.fillScreen(TFT_WHITE);

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
 * @brief Paleta de colores RGB332 cargada desde palette.bin
 */
static uint8_t PALETTE[256]; // Hasta 256 colores posibles. Se rellena al arrancar.
static uint32_t PALETTE_SIZE = 0;

static bool fillPolygons = true;

/**
 * @brief Cargar paleta RGB332 desde palette.bin en la raíz del directorio
 * @param palette_path Ruta completa al archivo palette.bin
 * @return true si OK, false si error
 */
bool loadPalette(const char* palette_path)
{
    FILE* f = fopen(palette_path, "rb");
    if (!f) {
        return false;
    }
    PALETTE_SIZE = fread(PALETTE, 1, 256, f); // Hasta 256 colores
    fclose(f);
    return PALETTE_SIZE > 0;
}

/**
 * @brief Convert palette index to RGB332 (usado en SET_COLOR_INDEX)
 */
uint8_t palette_index_to_rgb332(uint32_t idx)
{
    if (idx < PALETTE_SIZE)
        return PALETTE[idx];
    return 0xFF; // blanco por defecto si el índice es inválido
}

/**
 * @brief Oscurecer color RGB332 (para dibujar bordes de polígonos rellenos)
 */
uint8_t darken_rgb332(uint8_t c, float amount = 0.4f)
{
    uint8_t r = (c & 0xE0) >> 5;
    uint8_t g = (c & 0x1C) >> 2;
    uint8_t b = (c & 0x03);

    r = (uint8_t)(r * (1.0f - amount));
    g = (uint8_t)(g * (1.0f - amount));
    b = (uint8_t)(b * (1.0f - amount));

    return ((r << 5) | (g << 2) | b);
}

/**
 * @brief Convert RGB332 to RGB565 (SIN swap de bytes para ILI9488 con TFT_eSPI)
 */
static uint16_t rgb332_to_rgb565(uint8_t c)
{
    uint8_t r8 = (c & 0xE0);
    uint8_t g8 = (c & 0x1C) << 3;
    uint8_t b8 = (c & 0x03) << 6;
    uint16_t r5 = (r8 >> 3);
    uint16_t g6 = (g8 >> 2);
    uint16_t b5 = (b8 >> 3);
    uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
    return rgb565;
}

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
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    // Si offset supera dataSize, devolver 0 y dejar offset al final
    if (offset > dataSize) {
        offset = dataSize;
        return 0;
    }
    return value;
}

/**
 * @brief Read zigzag-encoded signed integer from binary data - FORMATO SCRIPT PYTHON
 */
static int32_t read_zigzag(const uint8_t* data, size_t& offset, size_t dataSize)
{
    if (offset >= dataSize) return 0;
    uint32_t encoded = read_varint(data, offset, dataSize);
    return (int32_t)((encoded >> 1) ^ (-(int32_t)(encoded & 1)));
}


/**
 * @brief Convert uint16 coordinate to tile pixel (0-255) - CORREGIDO PARA COORDENADAS 0-255
 */
static int uint16_to_tile_pixel(int32_t val) {
    int p = (int)((val * 256) / 65536);
    if (p < 0) p = 0;
    if (p > 255) p = 255;
    return p;
}

/**
 * @brief Check if point is on tile margin (ahora margen ±1 píxel)
 */
static bool isPointOnMargin(int px, int py)
{
    return (px <= 1 || px >= 254 || py <= 1 || py >= 254);
}

/**
 * @brief Check if line should be drawn (filtra bordes de tile, margen ±1)
 */
static bool shouldDrawLine(int px1, int py1, int px2, int py2)
{
    // Si ambos extremos están en el margen, NO dibujar
    if (isPointOnMargin(px1, py1) && isPointOnMargin(px2, py2)) {
        return false;
    }
    // Si el segmento es exactamente horizontal/vertical en el margen, NO dibujar
    if ((px1 == px2) && (px1 <= 1 || px1 >= 254)) return false;
    if ((py1 == py2) && (py1 <= 1 || py1 >= 254)) return false;
    return true;
}

/**
 * @brief Helper para rellenar polígonos generales (convexos y cóncavos) con TFT_eSPI (scanline fill)
 */
void fillPolygonGeneral(TFT_eSprite &map, int *px, int *py, int n, uint16_t color, int xOffset, int yOffset)
{
    if (n < 3 || n > 256) return;

    // Añadido: Si es un triángulo, usar fillTriangle (optimización fase 3 punto 5)
    if (n == 3) {
        map.fillTriangle(
            px[0] + xOffset, py[0] + yOffset,
            px[1] + xOffset, py[1] + yOffset,
            px[2] + xOffset, py[2] + yOffset,
            color
        );
        return;
    }

    int minY = py[0], maxY = py[0];
    for (int i = 1; i < n; ++i) {
        if (py[i] < minY) minY = py[i];
        if (py[i] > maxY) maxY = py[i];
    }

    bool touchesMargin = false;
    for (int i = 0; i < n; ++i) {
        if (isPointOnMargin(px[i], py[i])) {
            touchesMargin = true;
            break;
        }
    }
    if (touchesMargin) {
        minY = 0;
        maxY = 255;
    }
    if (minY < 0) minY = 0;
    if (maxY > 255) maxY = 255;

    for (int y = minY; y <= maxY; ++y) {
        int xints[256];
        int nints = 0;

        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            int yi = py[i], yj = py[j];
            int xi = px[i], xj = px[j];
            if ((yi < y && yj >= y) || (yj < y && yi >= y)) {
                if (yj != yi) {
                    int x = xi + (y - yi) * (xj - xi) / (yj - yi);
                    if (x < 0) x = 0;
                    if (x > 255) x = 255;
                    xints[nints++] = x;
                }
            }
        }

        for (int i = 0; i < nints - 1; ++i) {
            for (int j = i + 1; j < nints; ++j) {
                if (xints[i] > xints[j]) {
                    int tmp = xints[i]; xints[i] = xints[j]; xints[j] = tmp;
                }
            }
        }

        for (int i = 0; i < nints - 1; i += 2) {
            int xStart = xints[i], xEnd = xints[i+1];
            if (xStart < xEnd) {
                if (xStart < 0) xStart = 0;
                if (xEnd > 255) xEnd = 255;
                map.drawLine(xStart + xOffset, y + yOffset, xEnd + xOffset, y + yOffset, color);
            }
        }
    }
}

/**
 * @brief Dibuja el borde del polígono, asegurando que segmentos en el margen se dibujan con color de relleno
 */
void drawPolygonBorder(TFT_eSprite &map, int *px, int *py, int num_points, uint16_t borderColor, uint16_t fillColor, int xOffset, int yOffset)
{
    for (uint32_t i = 0; i < num_points - 1; ++i) {
        bool marginA = isPointOnMargin(px[i], py[i]);
        bool marginB = isPointOnMargin(px[i+1], py[i+1]);
        uint16_t color = (marginA && marginB) ? fillColor : borderColor;

        int x0 = px[i] + xOffset;
        int y0 = py[i] + yOffset;
        int x1 = px[i+1] + xOffset;
        int y1 = py[i+1] + yOffset;

        // Ajuste visual solo si ambos extremos están en margen:
        // si solo uno está en margen, NO lo movemos (evita líneas blancas)
        if (marginA && marginB) {
            // Si ambos están en margen, no hay ajuste adicional
        }
        // Si solo uno está en margen, NO mover el extremo

        // Solo dibujar si los puntos están dentro de tile
        if (x0 >= 0 && x0 <= 255 + xOffset && y0 >= 0 && y0 <= 255 + yOffset &&
            x1 >= 0 && x1 <= 255 + xOffset && y1 >= 0 && y1 <= 255 + yOffset) {
            map.drawLine(x0, y0, x1, y1, color);
        }
    }
    // Cierre del polígono
    bool marginA = isPointOnMargin(px[num_points-1], py[num_points-1]);
    bool marginB = isPointOnMargin(px[0], py[0]);
    uint16_t color = (marginA && marginB) ? fillColor : borderColor;
    int x0 = px[num_points-1] + xOffset;
    int y0 = py[num_points-1] + yOffset;
    int x1 = px[0] + xOffset;
    int y1 = py[0] + yOffset;

    if (x0 >= 0 && x0 <= 255 + xOffset && y0 >= 0 && y0 <= 255 + yOffset &&
        x1 >= 0 && x1 <= 255 + xOffset && y1 >= 0 && y1 <= 255 + yOffset) {
        map.drawLine(x0, y0, x1, y1, color);
    }
}

/**
 * @brief Renders a vector tile with ALL possible drawing commands
 */
bool Maps::renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map)
{
    static bool palette_loaded = false;
    if (!palette_loaded) {
        palette_loaded = loadPalette("/sdcard/VECTMAP/palette.bin");
    }

    if (!path || path[0] == '\0') {
        return false;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = nullptr;
#if BOARD_HAS_PSRAM
    data = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM);
#endif
    if (!data) {
        data = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_8BIT);
    }
    if (!data) {
        fclose(file);
        return false;
    }

    size_t bytesRead = fread(data, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) {
        heap_caps_free(data);
        return false;
    }

    size_t offset = 0;
    size_t dataSize = fileSize;
    uint8_t current_color = 0xFF;
    uint16_t currentDrawColor = rgb332_to_rgb565(current_color);

    uint32_t num_cmds = read_varint(data, offset, dataSize);
    if (num_cmds == 0) {
        heap_caps_free(data);
        return false;
    }

    int* px_list = (int*)heap_caps_malloc(512 * sizeof(int), MALLOC_CAP_8BIT);
    int* py_list = (int*)heap_caps_malloc(512 * sizeof(int), MALLOC_CAP_8BIT);

    if (!px_list || !py_list) {
        heap_caps_free(data);
        if (px_list) heap_caps_free(px_list);
        if (py_list) heap_caps_free(py_list);
        return false;
    }

    LineSegment* lineBatch = (LineSegment*)heap_caps_malloc(LINE_BATCH_SIZE * sizeof(LineSegment), MALLOC_CAP_8BIT);
    int batchCount = 0;
    if (!lineBatch) {
        heap_caps_free(data);
        heap_caps_free(px_list);
        heap_caps_free(py_list);
        return false;
    }

    auto flushBatch = [&]() {
        for (int i = 0; i < batchCount; ++i) {
            map.drawLine(lineBatch[i].x0, lineBatch[i].y0, lineBatch[i].x1, lineBatch[i].y1, lineBatch[i].color);
        }
        batchCount = 0;
    };

    int executed = 0;
    for (uint32_t cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) {
        if (offset >= dataSize) break;
        size_t cmd_start_offset = offset;
        uint32_t cmd_type = read_varint(data, offset, dataSize);

        bool isLineCommand = false;

        switch (cmd_type) {
            case 0x80:
                flushBatch();
                if (offset < dataSize) {
                    current_color = data[offset++];
                    currentDrawColor = rgb332_to_rgb565(current_color);
                    executed++;
                }
                break;
            case 0x81:
                flushBatch();
                {
                    uint32_t color_index = read_varint(data, offset, dataSize);
                    current_color = palette_index_to_rgb332(color_index);
                    currentDrawColor = rgb332_to_rgb565(current_color);
                    executed++;
                }
                break;
            case 1:
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    int32_t x2 = x1 + dx;
                    int32_t y2 = y1 + dy;
                    int px1 = uint16_to_tile_pixel(x1) + xOffset;
                    int py1 = uint16_to_tile_pixel(y1) + yOffset;
                    int px2 = uint16_to_tile_pixel(x2) + xOffset;
                    int py2 = uint16_to_tile_pixel(y2) + yOffset;
                    if (px1 >= 0 && px1 <= 255 + xOffset && py1 >= 0 && py1 <= 255 + yOffset &&
                        px2 >= 0 && px2 <= 255 + xOffset && py2 >= 0 && py2 <= 255 + yOffset) {
                        if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) {
                            if (batchCount < LINE_BATCH_SIZE) {
                                lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                            } else {
                                flushBatch();
                                lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                            }
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case 2:
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    if (num_points >= 2 && num_points <= 256) {
                        int32_t prevX = read_zigzag(data, offset, dataSize);
                        int32_t prevY = read_zigzag(data, offset, dataSize);
                        int prevPx = uint16_to_tile_pixel(prevX) + xOffset;
                        int prevPy = uint16_to_tile_pixel(prevY) + yOffset;
                        for (uint32_t i = 1; i < num_points; ++i) {
                            int32_t deltaX = read_zigzag(data, offset, dataSize);
                            int32_t deltaY = read_zigzag(data, offset, dataSize);
                            prevX += deltaX;
                            prevY += deltaY;
                            int currentPx = uint16_to_tile_pixel(prevX) + xOffset;
                            int currentPy = uint16_to_tile_pixel(prevY) + yOffset;
                            if (prevPx >= 0 && prevPx <= 255 + xOffset && prevPy >= 0 && prevPy <= 255 + yOffset &&
                                currentPx >= 0 && currentPx <= 255 + xOffset && currentPy >= 0 && currentPy <= 255 + yOffset) {
                                if (shouldDrawLine(prevPx - xOffset, prevPy - yOffset, currentPx - xOffset, currentPy - yOffset)) {
                                    if (batchCount < LINE_BATCH_SIZE) {
                                        lineBatch[batchCount++] = {prevPx, prevPy, currentPx, currentPy, currentDrawColor};
                                    } else {
                                        flushBatch();
                                        lineBatch[batchCount++] = {prevPx, prevPy, currentPx, currentPy, currentDrawColor};
                                    }
                                }
                            }
                            prevPx = currentPx;
                            prevPy = currentPy;
                        }
                        executed++;
                        isLineCommand = true;
                    } else {
                        for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                            read_zigzag(data, offset, dataSize);
                            read_zigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case 3:
                flushBatch();
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    if (num_points >= 3 && num_points <= 256) {
                        int32_t firstX = read_zigzag(data, offset, dataSize);
                        int32_t firstY = read_zigzag(data, offset, dataSize);
                        px_list[0] = uint16_to_tile_pixel(firstX);
                        py_list[0] = uint16_to_tile_pixel(firstY);
                        int prevX = firstX;
                        int prevY = firstY;
                        for (uint32_t i = 1; i < num_points; ++i) {
                            int32_t deltaX = read_zigzag(data, offset, dataSize);
                            int32_t deltaY = read_zigzag(data, offset, dataSize);
                            prevX += deltaX;
                            prevY += deltaY;
                            px_list[i] = uint16_to_tile_pixel(prevX);
                            py_list[i] = uint16_to_tile_pixel(prevY);
                        }
                        if (fillPolygons && num_points >= 3) {
                            fillPolygonGeneral(map, px_list, py_list, num_points, currentDrawColor, xOffset, yOffset);
                        }
                        uint16_t borderColor = rgb332_to_rgb565(darken_rgb332(current_color));
                        drawPolygonBorder(map, px_list, py_list, num_points, borderColor, currentDrawColor, xOffset, yOffset);
                        executed++;
                    } else {
                        for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                            read_zigzag(data, offset, dataSize);
                            read_zigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case 4:
                flushBatch();
                {
                    uint32_t num_points = read_varint(data, offset, dataSize);
                    int32_t accumX = 0, accumY = 0;
                    if (num_points >= 3 && num_points <= 256) {
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
                            px_list[i] = uint16_to_tile_pixel(accumX);
                            py_list[i] = uint16_to_tile_pixel(accumY);
                        }
                        if (fillPolygons && num_points >= 3) {
                            fillPolygonGeneral(map, px_list, py_list, num_points, currentDrawColor, xOffset, yOffset);
                            uint16_t borderColor = rgb332_to_rgb565(darken_rgb332(current_color));
                            drawPolygonBorder(map, px_list, py_list, num_points, borderColor, currentDrawColor, xOffset, yOffset);
                            executed++;
                        }
                    } else {
                        for (uint32_t i = 0; i < num_points && offset < dataSize; ++i) {
                            read_zigzag(data, offset, dataSize);
                            read_zigzag(data, offset, dataSize);
                        }
                    }
                }
                break;
            case 5:
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t y = read_zigzag(data, offset, dataSize);
                    int32_t x2 = x1 + dx;
                    int px1 = uint16_to_tile_pixel(x1) + xOffset;
                    int px2 = uint16_to_tile_pixel(x2) + xOffset;
                    int py = uint16_to_tile_pixel(y) + yOffset;
                    if (px1 >= 0 && px1 <= 255 + xOffset && px2 >= 0 && px2 <= 255 + xOffset && py >= 0 && py <= 255 + yOffset) {
                        if (shouldDrawLine(px1 - xOffset, py - yOffset, px2 - xOffset, py - yOffset)) {
                            if (batchCount < LINE_BATCH_SIZE) {
                                lineBatch[batchCount++] = {px1, py, px2, py, currentDrawColor};
                            } else {
                                flushBatch();
                                lineBatch[batchCount++] = {px1, py, px2, py, currentDrawColor};
                            }
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case 6:
                {
                    int32_t x = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    int32_t y2 = y1 + dy;
                    int px = uint16_to_tile_pixel(x) + xOffset;
                    int py1 = uint16_to_tile_pixel(y1) + yOffset;
                    int py2 = uint16_to_tile_pixel(y2) + yOffset;
                    if (px >= 0 && px <= 255 + xOffset && py1 >= 0 && py1 <= 255 + yOffset && py2 >= 0 && py2 <= 255 + yOffset) {
                        if (shouldDrawLine(px - xOffset, py1 - yOffset, px - xOffset, py2 - yOffset)) {
                            if (batchCount < LINE_BATCH_SIZE) {
                                lineBatch[batchCount++] = {px, py1, px, py2, currentDrawColor};
                            } else {
                                flushBatch();
                                lineBatch[batchCount++] = {px, py1, px, py2, currentDrawColor};
                            }
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case 0x82:
                flushBatch();
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    int px1 = uint16_to_tile_pixel(x1);
                    int py1 = uint16_to_tile_pixel(y1);
                    int pwidth = uint16_to_tile_pixel(dx);
                    int pheight = uint16_to_tile_pixel(dy);
                    if (px1 >= 0 && px1 <= 255 && py1 >= 0 && py1 <= 255 &&
                        pwidth > 0 && pheight > 0 &&
                        !isPointOnMargin(px1, py1) &&
                        !isPointOnMargin(px1 + pwidth, py1 + pheight)) {
                        if (fillPolygons) {
                            map.fillRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                            uint16_t borderColor = rgb332_to_rgb565(darken_rgb332(current_color));
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, borderColor);
                        } else {
                            map.drawRect(px1 + xOffset, py1 + yOffset, pwidth, pheight, currentDrawColor);
                        }
                        executed++;
                    }
                }
                break;
            case 0x83:
                {
                    int32_t x1 = read_zigzag(data, offset, dataSize);
                    int32_t y1 = read_zigzag(data, offset, dataSize);
                    int32_t dx = read_zigzag(data, offset, dataSize);
                    int32_t dy = read_zigzag(data, offset, dataSize);
                    int32_t x2 = x1 + dx;
                    int32_t y2 = y1 + dy;
                    int px1 = uint16_to_tile_pixel(x1) + xOffset;
                    int py1 = uint16_to_tile_pixel(y1) + yOffset;
                    int px2 = uint16_to_tile_pixel(x2) + xOffset;
                    int py2 = uint16_to_tile_pixel(y2) + yOffset;
                    if (px1 >= 0 && px1 <= 255 + xOffset && py1 >= 0 && py1 <= 255 + yOffset &&
                        px2 >= 0 && px2 <= 255 + xOffset && py2 >= 0 && py2 <= 255 + yOffset) {
                        if (shouldDrawLine(px1 - xOffset, py1 - yOffset, px2 - xOffset, py2 - yOffset)) {
                            if (batchCount < LINE_BATCH_SIZE) {
                                lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                            } else {
                                flushBatch();
                                lineBatch[batchCount++] = {px1, py1, px2, py2, currentDrawColor};
                            }
                        }
                        executed++;
                        isLineCommand = true;
                    }
                }
                break;
            case 0x87:
                flushBatch();
                {
                    int32_t center_x = read_zigzag(data, offset, dataSize);
                    int32_t center_y = read_zigzag(data, offset, dataSize);
                    int32_t radius = read_zigzag(data, offset, dataSize);
                    int pcx = uint16_to_tile_pixel(center_x);
                    int pcy = uint16_to_tile_pixel(center_y);
                    int pradius = uint16_to_tile_pixel(radius);
                    if (pcx >= 0 && pcx <= 255 && pcy >= 0 && pcy <= 255 && pradius > 0 &&
                        !isPointOnMargin(pcx, pcy)) {
                        if (fillPolygons) {
                            map.fillCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                            uint16_t borderColor = rgb332_to_rgb565(darken_rgb332(current_color));
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, borderColor);
                        } else {
                            map.drawCircle(pcx + xOffset, pcy + yOffset, pradius, currentDrawColor);
                        }
                        executed++;
                    }
                }
                break;
            default:
                flushBatch();
                if (offset < dataSize - 4) {
                    offset += 4;
                }
                break;
        }
        if (!isLineCommand) {
            flushBatch();
        }
        if (offset <= cmd_start_offset) break;
    }
    flushBatch();

    heap_caps_free(data);
    heap_caps_free(px_list);
    heap_caps_free(py_list);
    heap_caps_free(lineBatch);

    if (executed == 0) {
        return false;
    }
    return true;
}