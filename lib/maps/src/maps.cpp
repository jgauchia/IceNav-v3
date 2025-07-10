/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @author @aresta - https://github.com/aresta/ESP32_GPS - Vector Maps
 * @brief  Maps draw class
 * @version 0.2.3
 * @date 2025-06
 */

#include "maps.hpp"

extern Compass compass;
extern Gps gps;
extern Storage storage;
extern std::vector<wayPoint> trackData; /**< Vector containing track waypoints */
const char* TAG PROGMEM = "Maps";

/**
 * @brief Constructs a Point16 object from a comma-separated coordinate string.
 *
 * @details Parses a string containing two coordinates separated by a comma (e.g., "123,456"),
 * 			and initializes the x and y members accordingly.
 *
 * @param coordsPair Pointer to a null-terminated string with two coordinates.
 */
extern Point16::Point16(char *coordsPair)
{
	char *next;
	x = static_cast<int16_t>(strtol(coordsPair, &next, 10)); // 1st coord 
	y = static_cast<int16_t>(strtol(++next, nullptr, 10));  // 2nd coord
}

/**
 * @brief Checks if the bounding box contains a given point.
 *
 * @param p The point to check.
 * @return true if the point is inside the bounding box, false otherwise.
 */
bool BBox::containsPoint(const Point32 p) { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }

/**
 * @brief Checks if this bounding box intersects with another bounding box.
 *
 * @param b The bounding box to check intersection with.
 * @return true if the bounding boxes intersect, false otherwise.
 */
bool BBox::intersects(BBox b) const
{
	if (b.min.x > max.x || b.max.x < min.x || b.min.y > max.y || b.max.y < min.y)
		return false;
	return true;
}

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
uint32_t Maps::lon2tilex(double f_lon, uint8_t zoom)
{
    double rawTile = (f_lon + 180.0) / 360.0 * (1 << zoom);
    rawTile += 1e-6; 
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
uint32_t Maps::lat2tiley(double f_lat, uint8_t zoom)
{
    double lat_rad = f_lat * M_PI / 180.0;
    double siny = tan(lat_rad) + 1.0 / cos(lat_rad);
    double merc_n = log(siny);

    double rawTile = (1.0 - merc_n / M_PI) / 2.0 * (1 << zoom);
    rawTile += 1e-6;
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
double Maps::tilex2lon(uint32_t tileX, uint8_t zoom)
{
	return (double)tileX * 360.0 / (1 << zoom) - 180.0;
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
double Maps::tiley2lat(uint32_t tileY, uint8_t zoom)
{
	double scale = 1 << zoom;
	double n = M_PI * (1.0 - 2.0 * tileY / scale);
	return 180.0 / M_PI * atan(sinh(n));
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
Maps::MapTile Maps::getMapTile(double lon, double lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY)
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

// Vector Map Private section

/**
 * @brief Get pixel Y position from OpenStreetMap Vector map latitude
 *
 * @details Converts a latitude value to the corresponding Y position in the OpenStreetMap vector map
 * 			projection using the Mercator formula.
 *
 * @param lat Latitude coordinate.
 * @return Y position.
 */
double Maps::lat2y(double lat)
{
    constexpr double INV_DEG = M_PI / 180.0;
    constexpr double OFFSET = M_PI / 4.0;

    double rad = lat * INV_DEG;
    return log(tan(rad / 2.0 + OFFSET)) * EARTH_RADIUS;
}

/**
 * @brief Get pixel X position from OpenStreetMap Vector map longitude
 *
 * @details Converts a longitude value to the corresponding X position in the OpenStreetMap vector map
 * 			projection using the Mercator formula.
 *
 * @param lon Longitude coordinate.
 * @return X position.
 */
double Maps::lon2x(double lon)
{
  	return DEG2RAD(lon) * EARTH_RADIUS;
}

/**
 * @brief Get longitude from X position in Vector Map (Mercator projection)
 *
 * @details Converts an X position in the Mercator projection to the corresponding longitude value.
 *
 * @param x X position.
 * @return Longitude coordinate.
 */
double Maps::mercatorX2lon(double x)
{
	return (x / EARTH_RADIUS) * (180.0 / M_PI);
}

/**
 * @brief Get latitude from Y position in Vector Map (Mercator projection)
 *
 * @details Converts a Y position in the Mercator projection to the corresponding latitude value.
 *
 * @param y Y position.
 * @return Latitude coordinate.
 */
double Maps::mercatorY2lat(double y)
{
  	return (atan(sinh(y / EARTH_RADIUS))) * (180.0 / M_PI);
}

/**
 * @brief Points to screen coordinates
 *
 * @details Converts a map coordinate to a screen coordinate based on the current zoom and screen center.
 *
 * @param pxy Map coordinate (X or Y).
 * @param screenCenterxy Screen center coordinate (X or Y).
 * @return Screen coordinate as int16_t.
 */
int16_t Maps::toScreenCoord(const int32_t pxy, const int32_t screenCenterxy)
{
 	 return round((double)(pxy - screenCenterxy) / zoom) + (double)Maps::tileWidth / 2;
}

/**
 * @brief Returns int16 or 0 if empty
 *
 * @details Parses an integer value from the file buffer starting at Maps::idx. Returns the parsed int16_t value,
 * 			or 0 if the field is empty (next char is newline). Handles parsing errors and logs them.
 *
 * @param file Pointer to the character buffer to parse from.
 * @return Parsed int16_t value, 0 if empty, or -1 on error.
 */
int16_t Maps::parseInt16(char *file)
{
    char num[16];
    uint8_t i = 0;
    char c = file[Maps::idx];

    if (c == '\n')
        return 0;

    while (c >= '0' && c <= '9' && i < 15)
    {
        num[i++] = c;
        c = file[++Maps::idx];
    }
    num[i] = '\0';

    if (c != ';' && c != ',' && c != '\n')
    {
        ESP_LOGE(TAG, "parseInt16 error: %c %i", c, c);
        ESP_LOGE(TAG, "Num: [%s]", num);
        while (1);  
    }

    Maps::idx++;  

    try
    {
        return static_cast<int16_t>(std::stoi(num));
    }
    catch (const std::invalid_argument &)
    {
        ESP_LOGE(TAG, "parseInt16 invalid_argument: [%c] [%s]", c, num);
    }
    catch (const std::out_of_range &)
    {
        ESP_LOGE(TAG, "parseInt16 out_of_range: [%c] [%s]", c, num);
    }

    return -1;
}

/**
 * @brief Returns the string until terminator char or newline. The terminator character is not included but consumed from stream.
 *
 * @details Reads characters from the file buffer starting at Maps::idx into the provided string, 
 * 			stopping at the specified terminator character or newline. The terminator is not included 
 * 			in the result string but is consumed from the stream.
 *
 * @param file Pointer to the character buffer to parse from.
 * @param terminator Character to terminate the copy operation.
 * @param str Output buffer to store the parsed string (should be at least 30 bytes).
 */
void Maps::parseStrUntil(char *file, char terminator, char *str)
{
	uint8_t i = 0;
	char c;
	while ((c = file[Maps::idx]) != terminator && c != '\n')
	{
		assert(i < 29);
		str[i++] = c;
		Maps::idx++;
	}
	str[i] = '\0';
	Maps::idx++;
}

/**
 * @brief Parse vector file to coords
 *
 * @details Parses coordinate pairs from the provided file buffer and appends them to the points vector.
 *			Each coordinate pair is expected in the format "x,y;".
 *
 * @param file Pointer to the character buffer to parse from.
 * @param points Reference to a vector of Point16 to store the parsed points.
 */
void Maps::parseCoords(char *file, std::vector<Point16> &points)
{
	char str[30];
	assert(points.empty());

	while (true)
	{
		try
		{
			parseStrUntil(file, ',', str);
			if (!str[0])
				break;

			int16_t x = static_cast<int16_t>(std::stoi(str));

			parseStrUntil(file, ';', str);
			if (!str[0])
			{
				ESP_LOGE(TAG, "parseCoords missing Y coordinate");
				break;
			}

			int16_t y = static_cast<int16_t>(std::stoi(str));
			points.emplace_back(x, y);
		}
		catch (const std::invalid_argument&)
		{
			ESP_LOGE(TAG, "parseCoords invalid_argument: %s", str);
		}
		catch (const std::out_of_range&)
		{
			ESP_LOGE(TAG, "parseCoords out_of_range: %s", str);
		}
	}
}

/**
 * @brief Parse Mapbox
 *
 * @details Parses a bounding box (BBox) from a string containing four integer values separated by delimiters.
 *
 * @param str Input string containing the bounding box coordinates as integers.
 * @return BBox Parsed bounding box as a BBox object.
 */
BBox Maps::parseBbox(String str)
{
	const char* ptr = str.c_str();
	char* next;
	int32_t x1 = (int32_t)strtol(ptr, &next, 10);
	int32_t y1 = (int32_t)strtol(next + 1, &next, 10);
	int32_t x2 = (int32_t)strtol(next + 1, &next, 10);
	int32_t y2 = (int32_t)strtol(next + 1, nullptr, 10);
	return BBox(Point32(x1, y1), Point32(x2, y2));
}

/**
 * @brief Read vector map file to memory block
 *
 * @details Reads a vector map file (with .fmp extension) into a MapBlock structure, parsing polygons and polylines.
 *
 * @param fileName Name of the file (without extension).
 * @return MapBlock* Pointer to the allocated MapBlock structure, or with inView=false if not found.
 */
Maps::MapBlock *Maps::readMapBlock(String fileName)
{
	ESP_LOGI(TAG, "readMapBlock: %s", fileName.c_str());
	char str[30];
	MapBlock *mblock = new MapBlock();
	std::string filePath = fileName.c_str() + std::string(".fmp");
	ESP_LOGI(TAG, "File: %s", filePath.c_str());

	FILE *file_ = storage.open(filePath.c_str(), "r");

	if (!file_)
	{
		Maps::isMapFound = false;
		mblock->inView = false;
		return mblock;
	}
	else
	{
		size_t fileSize = storage.size(filePath.c_str());

	#ifdef BOARD_HAS_PSRAM
		char *file = (char *)ps_malloc(fileSize + 1);
	#else
		char *file = (char *)malloc(fileSize + 1);
	#endif

		vTaskDelay(1);  // Stabilize SD/MMC bus before read

		size_t bytesRead = storage.read(file_, file, fileSize);
		if (bytesRead != fileSize)
		{
			ESP_LOGE(TAG, "Error reading map file. Expected %u bytes, got %u", fileSize, bytesRead);
			storage.close(file_);
			delete[] file;
			mblock->inView = false;
			return mblock;
		}

		Maps::isMapFound = true;
		uint32_t line = 0;
		Maps::idx = 0;

		Maps::parseStrUntil(file, ':', str);
		if (strcmp(str, "Polygons") != 0)
		{
			ESP_LOGE(TAG, "Map error. Expected Polygons instead of: %s", str);
			while (true);
		}

		int16_t count = Maps::parseInt16(file);
		assert(count > 0);
		line++;

		uint32_t totalPoints = 0;
		Polygon polygon;
		Point16 p;
		while (count > 0)
		{
			Maps::parseStrUntil(file, '\n', str); // color
			assert(str[0] == '0' && str[1] == 'x');
			polygon.color = (uint16_t)std::stoul(str, nullptr, 16);
			line++;

			Maps::parseStrUntil(file, '\n', str); // maxZoom
			polygon.maxZoom = str[0] ? (uint8_t)std::stoi(str) : MAX_ZOOM;
			line++;

			Maps::parseStrUntil(file, ':', str);
			if (strcmp(str, "bbox") != 0)
			{
				ESP_LOGE(TAG, "bbox error tag. Line %i : %s", line, str);
				while (true);
			}
			polygon.bbox.min.x = Maps::parseInt16(file);
			polygon.bbox.min.y = Maps::parseInt16(file);
			polygon.bbox.max.x = Maps::parseInt16(file);
			polygon.bbox.max.y = Maps::parseInt16(file);
			line++;

			polygon.points.clear();
			Maps::parseStrUntil(file, ':', str);
			if (strcmp(str, "coords") != 0)
			{
				ESP_LOGE(TAG, "coords error tag. Line %i : %s", line, str);
				while (true);
			}
			Maps::parseCoords(file, polygon.points);
			line++;

			mblock->polygons.push_back(polygon);
			totalPoints += polygon.points.size();
			count--;
		}
		assert(count == 0);

		Maps::parseStrUntil(file, ':', str);
		if (strcmp(str, "Polylines") != 0)
			ESP_LOGE(TAG, "Map error. Expected Polylines instead of: %s", str);
		count = Maps::parseInt16(file);
		assert(count > 0);
		line++;

		Polyline polyline;
		while (count > 0)
		{
			Maps::parseStrUntil(file, '\n', str); // color
			assert(str[0] == '0' && str[1] == 'x');
			polyline.color = (uint16_t)std::stoul(str, nullptr, 16);
			line++;

			Maps::parseStrUntil(file, '\n', str); // width
			polyline.width = str[0] ? (uint8_t)std::stoi(str) : 1;
			line++;

			Maps::parseStrUntil(file, '\n', str); // maxZoom
			polyline.maxZoom = str[0] ? (uint8_t)std::stoi(str) : MAX_ZOOM;
			line++;

			Maps::parseStrUntil(file, ':', str);
			if (strcmp(str, "bbox") != 0)
			{
				ESP_LOGE(TAG, "bbox error tag. Line %i : %s", line, str);
				while (true);
			}
			polyline.bbox.min.x = Maps::parseInt16(file);
			polyline.bbox.min.y = Maps::parseInt16(file);
			polyline.bbox.max.x = Maps::parseInt16(file);
			polyline.bbox.max.y = Maps::parseInt16(file);
			line++;

			polyline.points.clear();
			Maps::parseStrUntil(file, ':', str);
			if (strcmp(str, "coords") != 0)
			{
				ESP_LOGI(TAG, "coords tag. Line %i : %s", line, str);
				while (true);
			}
			Maps::parseCoords(file, polyline.points);
			line++;

			mblock->polylines.push_back(polyline);
			totalPoints += polyline.points.size();
			count--;
		}
		assert(count == 0);

		storage.close(file_);
		delete[] file;
		return mblock;
	}
}

/**
 * @brief Fill polygon routine
 *
 * @details Fills the given polygon using the scanline fill algorithm. Each scanline finds intersections
 * 			with the polygon edges, sorts them, and draws horizontal lines between node pairs.
 *
 * @param p Polygon to fill.
 * @param map Reference to TFT_eSprite on which to draw.
 */
void Maps::fillPolygon(Polygon p, TFT_eSprite &map)
{
	int16_t maxY = p.bbox.max.y;
	int16_t minY = p.bbox.min.y;

	if (maxY >= Maps::tileHeight)
		maxY = Maps::tileHeight - 1;
	if (minY < 0)
		minY = 0;
	if (minY >= maxY)
		return;

	int16_t pixelY;
	std::vector<int16_t> nodeX;

	for (pixelY = minY; pixelY <= maxY; ++pixelY)
	{
		nodeX.clear();
		for (size_t i = 0; i < p.points.size() - 1; ++i)
		{
			int16_t y0 = p.points[i].y;
			int16_t y1 = p.points[i + 1].y;

			if ((y0 < pixelY && y1 >= pixelY) || (y1 < pixelY && y0 >= pixelY))
			{
				int16_t x0 = p.points[i].x;
				int16_t x1 = p.points[i + 1].x;

				float ratio = float(pixelY - y0) / float(y1 - y0);
				int16_t intersectX = x0 + (int16_t)(ratio * float(x1 - x0));
				nodeX.push_back(intersectX);
			}
		}

		if (nodeX.size() < 2)
			continue;

		std::sort(nodeX.begin(), nodeX.end());

		for (size_t i = 0; i + 1 < nodeX.size(); i += 2)
		{
			int16_t x0 = nodeX[i];
			int16_t x1 = nodeX[i + 1];

			if (x0 > Maps::tileWidth)
				break;
			if (x1 < 0)
				continue;
			if (x0 < 0)
				x0 = 0;
			if (x1 > Maps::tileWidth)
				x1 = Maps::tileWidth;

			map.drawLine(x0, Maps::tileHeight - pixelY, x1, Maps::tileHeight - pixelY, p.color);
		}
	}
}

/**
 * @brief Get bounding objects in memory block
 *
 * @details Ensures that all map blocks covering the corners of a given bounding box (bbox) are loaded into memory.
 *			If necessary, loads new blocks from SD Card and manages the memory cache, removing the oldest block if the cache is full.
 * 			Sets the 'inView' flag for blocks currently needed.
 *
 * @param bbox Bounding box specifying the region of interest.
 * @param memCache Reference to the memory cache holding loaded map blocks.
 */
void Maps::getMapBlocks(BBox &bbox, Maps::MemCache &memCache)
{
	ESP_LOGI(TAG, "getMapBlocks %i", millis());

	for (MapBlock *block : memCache.blocks)
		block->inView = false;

	const Point32 corners[4] = {
		bbox.min,
		bbox.max,
		Point32(bbox.min.x, bbox.max.y),
		Point32(bbox.max.x, bbox.min.y)};

	for (const Point32 &point : corners)
	{
		int32_t blockMinX = point.x & ~MAPBLOCK_MASK;
		int32_t blockMinY = point.y & ~MAPBLOCK_MASK;

		bool found = false;
		for (MapBlock *memblock : memCache.blocks)
		{
			if (memblock->offset.x == blockMinX && memblock->offset.y == blockMinY)
			{
				memblock->inView = true;
				found = true;
				break;
			}
		}
		if (found)
			continue;

		ESP_LOGI(TAG, "load from disk (%i, %i) %i", blockMinX, blockMinY, millis());

		int32_t folderNameX = blockMinX >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
		int32_t folderNameY = blockMinY >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
		int32_t blockX = (blockMinX >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;
		int32_t blockY = (blockMinY >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;

		char folderName[12];
		snprintf(folderName, sizeof(folderName), "%+04d%+04d", folderNameX, folderNameY);
		String fileName = mapVectorFolder + folderName + "/" + blockX + "_" + blockY;

		if (memCache.blocks.size() >= MAPBLOCKS_MAX)
		{
			ESP_LOGV(TAG, "Deleting block - freeHeap: %i", esp_get_free_heap_size());
			delete memCache.blocks.front();
			memCache.blocks.erase(memCache.blocks.begin());
			ESP_LOGV(TAG, "Deleted - freeHeap: %i", esp_get_free_heap_size());
		}

		MapBlock *newBlock = Maps::readMapBlock(fileName);
		if (Maps::isMapFound && newBlock)
		{
			newBlock->inView = true;
			newBlock->offset = Point32(blockMinX, blockMinY);
			memCache.blocks.push_back(newBlock);
			assert(memCache.blocks.size() <= MAPBLOCKS_MAX);

			ESP_LOGI(TAG, "Block loaded: %p", newBlock);
			ESP_LOGI(TAG, "FreeHeap: %i", esp_get_free_heap_size());
		}
	}

	ESP_LOGI(TAG, "memCache size: %i %i", memCache.blocks.size(), millis());
}

/**
 * @brief Generate vectorized map
 *
 * @details Renders the vector map using in-memory blocks within the current viewport and zoom level.
 * 			Draws polygons and polylines, updates map bounds, and overlays waypoints and tracks.
 *
 * @param viewPort Viewport describing the area to render.
 * @param memCache Memory cache holding loaded map blocks.
 * @param map Map sprite (TFT_eSprite) to draw on.
 * @param zoom Zoom level for rendering.
 */
void Maps::readVectorMap(Maps::ViewPort &viewPort, Maps::MemCache &memCache, TFT_eSprite &map, uint8_t zoom)
{
	Polygon newPolygon;
	map.fillScreen(BACKGROUND_COLOR);
	uint32_t totalTime = millis();
	ESP_LOGI(TAG, "Draw start %i", totalTime);

	if (!Maps::isMapFound)
	{
		Maps::isMapFound = false;
		map.fillScreen(TFT_BLACK);
		Maps::showNoMap(map);
		ESP_LOGE(TAG, "Map doesn't exist");
		return;
	}

	for (MapBlock *mblock : memCache.blocks)
	{
		if (!mblock->inView)
			continue;

		uint32_t blockTime = millis();

		const Point16 screen_center_mc = viewPort.center.toPoint16() - mblock->offset.toPoint16();
		const BBox screen_bbox_mc = viewPort.bbox - mblock->offset;

		for (const Polygon &polygon : mblock->polygons)
		{
			if (zoom > polygon.maxZoom || !polygon.bbox.intersects(screen_bbox_mc))
				continue;

			newPolygon.color = polygon.color;
			newPolygon.bbox.min.x = Maps::toScreenCoord(polygon.bbox.min.x, screen_center_mc.x);
			newPolygon.bbox.min.y = Maps::toScreenCoord(polygon.bbox.min.y, screen_center_mc.y);
			newPolygon.bbox.max.x = Maps::toScreenCoord(polygon.bbox.max.x, screen_center_mc.x);
			newPolygon.bbox.max.y = Maps::toScreenCoord(polygon.bbox.max.y, screen_center_mc.y);

			newPolygon.points.clear();
			newPolygon.points.reserve(polygon.points.size());

			for (const Point16 &p : polygon.points)
			{
				newPolygon.points.emplace_back(
					Maps::toScreenCoord(p.x, screen_center_mc.x),
					Maps::toScreenCoord(p.y, screen_center_mc.y));
			}

			Maps::fillPolygon(newPolygon, map);
		}
		ESP_LOGI(TAG, "Block polygons done %i ms", millis() - blockTime);
		blockTime = millis();

		for (const Polyline &line : mblock->polylines)
		{
			if (zoom > line.maxZoom || !line.bbox.intersects(screen_bbox_mc))
				continue;

			for (size_t i = 0; i < line.points.size() - 1; ++i)
			{
				const int16_t p1x = Maps::toScreenCoord(line.points[i].x, screen_center_mc.x);
				const int16_t p1y = Maps::toScreenCoord(line.points[i].y, screen_center_mc.y);
				const int16_t p2x = Maps::toScreenCoord(line.points[i + 1].x, screen_center_mc.x);
				const int16_t p2y = Maps::toScreenCoord(line.points[i + 1].y, screen_center_mc.y);

				map.drawLine(p1x, Maps::tileHeight - p1y, p2x, Maps::tileHeight - p2y, line.color);
			}
		}
		ESP_LOGI(TAG, "Block lines done %i ms", millis() - blockTime);
	}

	ESP_LOGI(TAG, "Total %i ms", millis() - totalTime);

	MapBlock *firstBlock = memCache.blocks.front();
	delete firstBlock;
	memCache.blocks.erase(memCache.blocks.begin());

	Maps::totalBounds.lat_min = Maps::mercatorY2lat(viewPort.bbox.min.y);
	Maps::totalBounds.lat_max = Maps::mercatorY2lat(viewPort.bbox.max.y);
	Maps::totalBounds.lon_min = Maps::mercatorX2lon(viewPort.bbox.min.x);
	Maps::totalBounds.lon_max = Maps::mercatorX2lon(viewPort.bbox.max.x);

	ESP_LOGI(TAG, "Total Bounds: Lat Min: %f, Lat Max: %f, Lon Min: %f, Lon Max: %f",
			 Maps::totalBounds.lat_min, Maps::totalBounds.lat_max,
			 Maps::totalBounds.lon_min, Maps::totalBounds.lon_max);

	if (Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
	{
		Maps::coords2map(destLat, destLon, Maps::totalBounds, &wptPosX, &wptPosY);
	}
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
			map.drawWideLine(x1, y1, x2, y2, 2, TFT_BLUE);
		}
	}
}

/**
 * @brief Get vector map position from GPS position and check if moved
 *
 * @details Updates the internal map position based on the provided GPS latitude and longitude.
 * 			If the position has changed significantly since the previous update, recalculates
 * 			the map's internal coordinates and sets the moved flag.
 *
 * @param lat Latitude in degrees.
 * @param lon Longitude in degrees.
 */
void Maps::getPosition(double lat, double lon)
{
	if (abs(lat - Maps::prevLat) > 0.00005 && abs(lon - Maps::prevLon) > 0.00005)
	{
		Maps::point.x = Maps::lon2x(lon);
		Maps::point.y = Maps::lat2y(lat);
		Maps::prevLat = lat;
		Maps::prevLon = lon;
		Maps::isPosMoved = true;
	}
}


// Common Private section

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
bool Maps::isCoordInBounds(double lat, double lon, tileBounds bound)
{
 	 return (lat >= Maps::totalBounds.lat_min && lat <= Maps::totalBounds.lat_max && lon >= Maps::totalBounds.lon_min && lon <= Maps::totalBounds.lon_max);
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
Maps::ScreenCoord Maps::coord2ScreenPos(double lon, double lat, uint8_t zoomLevel, uint16_t tileSize)
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
void Maps::coords2map(double lat, double lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY)
{
	double lon_ratio = (lon - bound.lon_min) / (bound.lon_max - bound.lon_min);
	double lat_ratio = (bound.lat_max - lat) / (bound.lat_max - bound.lat_min);

	*pixelX = (int)(lon_ratio * Maps::tileWidth);
	*pixelY = (int)(lat_ratio * Maps::tileHeight);
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
 * @brief Set center coordinates of viewport
 *
 * @details Sets the center of the viewport and updates the bounding box (bbox) based on the current zoom and tile dimensions.
 *
 * @param pcenter New center coordinates (Point32) for the viewport.
 */
void Maps::ViewPort::setCenter(Point32 pcenter)
{
	center = pcenter;
	bbox.min.x = pcenter.x - Maps::tileWidth * zoom / 2;
	bbox.min.y = pcenter.y - Maps::tileHeight * zoom / 2;
	bbox.max.x = pcenter.x + Maps::tileWidth * zoom / 2;
	bbox.max.y = pcenter.y + Maps::tileHeight * zoom / 2;
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
 * @brief Generate render map
 *
 * @details Generates the main rendered map by compositing the center and surrounding tiles based on the current zoom level.
 * 			Handles missing tiles, updates map bounds, overlays missing map notifications, and draws tracks if available.
 *
 * @param zoom Zoom Level
 */
void Maps::generateRenderMap(uint8_t zoom)
{
	Maps::mapTileSize = Maps::renderMapTileSize;
	Maps::zoomLevel = zoom;

	bool foundRoundMap = false;
	bool missingMap = false;

	const double lat = Maps::followGps ? gps.gpsData.latitude : Maps::currentMapTile.lat;
	const double lon = Maps::followGps ? gps.gpsData.longitude : Maps::currentMapTile.lon;

	Maps::currentMapTile = Maps::getMapTile(lon, lat, Maps::zoomLevel, 0, 0);

	// Detects if tile changes from actual GPS position
	if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 ||
		Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
		Maps::currentMapTile.tilex != Maps::oldMapTile.tilex ||
		Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
	{
		const int16_t size = Maps::mapTileSize;

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

					foundRoundMap = Maps::mapTempSprite.drawPngFile(
						Maps::roundMapTile.file, offsetX, offsetY);

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
 * @brief Generate vector map
 *
 * @details Generates the vector map for the current GPS position and zoom level.
 * 			If the position has moved, updates the viewport, fetches the relevant map blocks,
 * 			reads and renders the vector map, and then resets the movement flag.
 *
 * @param zoom Zoom Level
 */
void Maps::generateVectorMap(uint8_t zoom)
{
	const double lat = gps.gpsData.latitude;
	const double lon = gps.gpsData.longitude;

	Maps::getPosition(lat, lon);

	if (!Maps::isPosMoved)
		return;

	Maps::mapTileSize = Maps::vectorMapTileSize;
	Maps::zoomLevel = zoom;
	Maps::viewPort.setCenter(Maps::point);
	Maps::getMapBlocks(Maps::viewPort.bbox, Maps::memCache);
	Maps::readVectorMap(Maps::viewPort, Maps::memCache, Maps::mapTempSprite, zoom);
	Maps::isPosMoved = false;
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

	if (size == Maps::renderMapTileSize)
	{
		if (Maps::followGps)
		{
			const double lat = gps.gpsData.latitude;
			const double lon = gps.gpsData.longitude;
			Maps::navArrowPosition = Maps::coord2ScreenPos(lon, lat, Maps::zoomLevel, Maps::renderMapTileSize);
			Maps::mapTempSprite.setPivot(Maps::renderMapTileSize + Maps::navArrowPosition.posX,
										 Maps::renderMapTileSize + Maps::navArrowPosition.posY);
		}
		else
		{
			const int16_t pivotX = Maps::tileWidth / 2 + Maps::offsetX;
			const int16_t pivotY = Maps::tileHeight / 2 + Maps::offsetY;
			Maps::mapTempSprite.setPivot(pivotX, pivotY);
		}
	}
	else if (size == Maps::vectorMapTileSize)
	{
		Maps::mapTempSprite.setPivot(Maps::vectorMapTileSize, Maps::vectorMapTileSize);
	}

	if (Maps::followGps)
		Maps::mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
	else
		Maps::mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);
}

/**
 * @brief Set Waypoint coords in Map
 *
 * @details Sets the latitude and longitude for the waypoint on the map.
 *
 * @param wptLat Waypoint Latitude
 * @param wptLon Waypoint Longitude
 */
void Maps::setWaypoint(double wptLat, double wptLon)
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
	Maps::isPosMoved = true;
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
 void Maps::centerOnGps(double lat, double lon)
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
	const int16_t tileSize = Maps::renderMapTileSize;

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
	const int16_t tileSize = renderMapTileSize;
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
