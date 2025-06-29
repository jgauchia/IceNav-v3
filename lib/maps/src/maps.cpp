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
	x = (int16_t)round(strtod(coordsPair, &next)); // 1st coord // TODO: change by strtol and test
	y = (int16_t)round(strtod(++next, NULL));      // 2nd coord
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
bool BBox::intersects(BBox b)
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
 	 return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
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
  	return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
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
	double rawTile = (f_lon + 180.0) / 360.0 * pow(2.0, zoom);
	rawTile += 1e-6;
	return (uint32_t)(floor(rawTile));
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
	double rawTile = (1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, zoom);
	rawTile += 1e-6;
	return (uint32_t)(floor(rawTile));
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
	return tileX / pow(2.0, zoom) * 360.0 - 180.0;
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
	double n = M_PI - 2.0 * M_PI * tileY / pow(2.0, zoom);
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
	char tileFile[255];
	uint32_t x = Maps::lon2tilex(lon, zoomLevel) + offsetX;
	uint32_t y = Maps::lat2tiley(lat, zoomLevel) + offsetY;

	sprintf(tileFile, mapRenderFolder, zoomLevel, x, y);
	MapTile data;
	strcpy(data.file,tileFile);
	data.tilex = x;
	data.tiley = y;
	data.zoom = zoomLevel;
	data.lat = lat;
	data.lon = lon;
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
  	return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
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
	uint8_t i;
	char c;
	i = 0;
	c = file[Maps::idx];
	if (c == '\n')
		return 0;
	while (c >= '0' && c <= '9')
	{
		assert(i < 15);
		c = file[Maps::idx];
		num[i] = c;
		Maps::idx++;
		i++;

		c = file[Maps::idx];
	}
	num[i] = '\0';

	if (c != ';' && c != ',' && c != '\n')
	{
		ESP_LOGE(TAG, "parseInt16 error: %c %i", c, c);
		ESP_LOGE(TAG, "Num: [%s]", num);
		while (1);
	}
	try
	{
		Maps::idx++;
		return std::stoi(num);
	}
	catch (std::invalid_argument)
	{
		ESP_LOGE(TAG, "parseInt16 invalid_argument: [%c] [%s]", c, num);
	}
	catch (std::out_of_range)
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
	uint8_t i;
	char c;
	i = 0;
	c = file[Maps::idx];
	while (c != terminator && c != '\n')
	{
		assert(i < 29);
		str[i] = c;
		Maps::idx++;
		i++;
		c = file[Maps::idx];
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
	assert(points.size() == 0);
	Point16 point;
	while (true)
	{
		try
		{
			parseStrUntil(file, ',', str);
			if (str[0] == '\0')
				break;
			point.x = (int16_t)std::stoi(str);
			parseStrUntil(file, ';', str);
			assert(str[0] != '\0');
			point.y = (int16_t)std::stoi(str);
		}
		catch (std::invalid_argument)
		{
			ESP_LOGE(TAG, "parseCoords invalid_argument: %s", str);
		}
		catch (std::out_of_range)
		{
			ESP_LOGE(TAG, "parseCoords out_of_range: %s", str);
		}
		points.push_back(point);
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
	char *next;
	int32_t x1 = (int32_t)strtol(str.c_str(), &next, 10);
	int32_t y1 = (int32_t)strtol(++next, &next, 10);
	int32_t x2 = (int32_t)strtol(++next, &next, 10);
	int32_t y2 = (int32_t)strtol(++next, NULL, 10);
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

		storage.read(file_, file, fileSize);
		Maps::isMapFound = true;

		uint32_t line = 0;
		Maps::idx = 0;

		// read polygons
		Maps::parseStrUntil(file, ':', str);
		if (strcmp(str, "Polygons") != 0)
		{
			ESP_LOGE(TAG, "Map error. Expected Polygons instead of: %s", str);
			while (0);
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

		// read lines
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
void Maps::fillPolygon(Polygon p, TFT_eSprite &map) // scanline fill algorithm
{
	int16_t maxY = p.bbox.max.y;
	int16_t minY = p.bbox.min.y;

	if (maxY >= Maps::tileHeight)
		maxY = Maps::tileHeight - 1;
	if (minY < 0)
		minY = 0;
	if (minY >= maxY)
		return;

	int16_t nodeX[p.points.size()], pixelY;

	//  Loop through the rows of the image.
	int16_t nodes, i, swap;
	for (pixelY = minY; pixelY <= maxY; pixelY++)
	{ //  Build a list of nodes.
		nodes = 0;
		for (int i = 0; i < (p.points.size() - 1); i++)
		{
			if ((p.points[i].y < pixelY && p.points[i + 1].y >= pixelY) ||
				(p.points[i].y >= pixelY && p.points[i + 1].y < pixelY))
			{
				nodeX[nodes++] =
								p.points[i].x + double(pixelY - p.points[i].y) / double(p.points[i + 1].y - p.points[i].y) *
								double(p.points[i + 1].x - p.points[i].x);
			}
		}
		assert(nodes < p.points.size());

		//  Sort the nodes, via a simple “Bubble” sort.
		i = 0;
		while (i < nodes - 1)
		{ // TODO: rework
			if (nodeX[i] > nodeX[i + 1])
			{
				swap = nodeX[i];
				nodeX[i] = nodeX[i + 1];
				nodeX[i + 1] = swap;
				i = 0;
			}
			else
				i++;
		}

		//  Fill the pixels between node pairs.
		for (i = 0; i <= nodes - 2; i += 2)
		{
			if (nodeX[i] > Maps::tileWidth)
				break;
			if (nodeX[i + 1] < 0)
				continue;
			if (nodeX[i] < 0)
				nodeX[i] = 0;
			if (nodeX[i + 1] > Maps::tileWidth)
				nodeX[i + 1] = Maps::tileWidth;
			map.drawLine(nodeX[i], Maps::tileHeight - pixelY, nodeX[i + 1], Maps::tileHeight - pixelY, p.color);
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
	{
		block->inView = false;
	}
	// loop the 4 corners of the bbox and find the files that contain them
	for (Point32 point : {bbox.min, bbox.max, Point32(bbox.min.x, bbox.max.y), Point32(bbox.max.x, bbox.min.y)})
	{
		bool found = false;
		int32_t blockMinX = point.x & (~MAPBLOCK_MASK);
		int32_t blockMinY = point.y & (~MAPBLOCK_MASK);

		// check if the needed block is already in memory
		for (MapBlock *memblock : memCache.blocks)
		{
			if (blockMinX == memblock->offset.x && blockMinY == memblock->offset.y)
			{
				memblock->inView = true;
				found = true;
				break;
			}
		}
		if (found)
			continue;

		ESP_LOGI(TAG, "load from disk (%i, %i) %i", blockMinX, blockMinY, millis());
		// block is not in memory => load from disk
		int32_t blockX = (blockMinX >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;
		int32_t blockY = (blockMinY >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;
		int32_t folderNameX = blockMinX >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
		int32_t folderNameY = blockMinY >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
		char folderName[12];
		snprintf(folderName, 9, "%+04d%+04d", folderNameX, folderNameY);              // force sign and 4 chars per number
		String fileName = mapVectorFolder + folderName + "/" + blockX + "_" + blockY; //  /maps/123_456/777_888

		// check if cache is full
		if (memCache.blocks.size() >= MAPBLOCKS_MAX)
		{
			// remove first one, the oldest
			ESP_LOGV(TAG, "Deleting freeHeap: %i", esp_get_free_heap_size());
			MapBlock *firstBlock = memCache.blocks.front();
			delete firstBlock;                              // free memory
			memCache.blocks.erase(memCache.blocks.begin()); // remove pointer from the vector
			ESP_LOGV(TAG, "Deleted freeHeap: %i", esp_get_free_heap_size());
		}

		MapBlock *newBlock = Maps::readMapBlock(fileName);
		if (Maps::isMapFound)
		{
			newBlock->inView = true;
			newBlock->offset = Point32(blockMinX, blockMinY);
			memCache.blocks.push_back(newBlock); // add the block to the memory cache
			assert(memCache.blocks.size() <= MAPBLOCKS_MAX);

			ESP_LOGI(TAG, "Block readed from SD card: %p", newBlock);
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
	int16_t p1x, p1y, p2x, p2y;
	if (Maps::isMapFound)
	{
		for (MapBlock *mblock : memCache.blocks)
		{
			uint32_t blockTime = millis();
			if (!mblock->inView)
				continue;

			// block to draw
			Point16 screen_center_mc = viewPort.center.toPoint16() - mblock->offset.toPoint16(); // screen center with features coordinates
			BBox screen_bbox_mc = viewPort.bbox - mblock->offset;                                // screen boundaries with features coordinates

			////// Polygons
			for (Polygon polygon : mblock->polygons)
			{
				if (zoom > polygon.maxZoom)
					continue;
				if (!polygon.bbox.intersects(screen_bbox_mc))
					continue;
				newPolygon.color = polygon.color;
				newPolygon.bbox.min.x = Maps::toScreenCoord(polygon.bbox.min.x, screen_center_mc.x);
				newPolygon.bbox.min.y = Maps::toScreenCoord(polygon.bbox.min.y, screen_center_mc.y);
				newPolygon.bbox.max.x = Maps::toScreenCoord(polygon.bbox.max.x, screen_center_mc.x);
				newPolygon.bbox.max.y = Maps::toScreenCoord(polygon.bbox.max.y, screen_center_mc.y);

				newPolygon.points.clear();
				for (Point16 p : polygon.points)
				{ // TODO: move to fillPolygon
					newPolygon.points.push_back(Point16(
						Maps::toScreenCoord(p.x, screen_center_mc.x),
						Maps::toScreenCoord(p.y, screen_center_mc.y)));
				}
				Maps::fillPolygon(newPolygon, map);
			}
			ESP_LOGI(TAG, "Block polygons done %i ms", millis() - blockTime);
			blockTime = millis();

			////// Lines
			for (Polyline line : mblock->polylines)
			{
				if (zoom > line.maxZoom)
					continue;
				if (!line.bbox.intersects(screen_bbox_mc))
					continue;

				p1x = -1;
				for (int i = 0; i < line.points.size() - 1; i++)
				{ // TODO optimize
					p1x = Maps::toScreenCoord(line.points[i].x, screen_center_mc.x);
					p1y = Maps::toScreenCoord(line.points[i].y, screen_center_mc.y);
					p2x = Maps::toScreenCoord(line.points[i + 1].x, screen_center_mc.x);
					p2y = Maps::toScreenCoord(line.points[i + 1].y, screen_center_mc.y);
					// map.drawWideLine(
					//                     p1x, TILE_HEIGHT - p1y,
					//                     p2x, TILE_HEIGHT - p2y,
					//                     line.width / zoom ?: 1, line.color);
					map.drawLine(p1x, Maps::tileHeight - p1y, p2x, Maps::tileHeight - p2y, line.color);
				}
			}
			ESP_LOGI(TAG, "Block lines done %i ms", millis() - blockTime);
		}
		ESP_LOGI(TAG, "Total %i ms", millis() - totalTime);

		// TODO: paint only in NAV mode
		// map.fillTriangle(
		//     TILE_WIDTH / 2 - 4, TILE_HEIGHT / 2 + 5,
		//     TILE_WIDTH / 2 + 4, TILE_HEIGHT / 2 + 5,
		//     TILE_WIDTH / 2, TILE_HEIGHT / 2 - 6,
		//     RED);
		ESP_LOGI(TAG, "Draw done! %i", millis());

		MapBlock *firstBlock = memCache.blocks.front();
		delete firstBlock;
		memCache.blocks.erase(memCache.blocks.begin());

		Maps::totalBounds.lat_min = Maps::mercatorY2lat(viewPort.bbox.min.y);
		Maps::totalBounds.lat_max = Maps::mercatorY2lat(viewPort.bbox.max.y);
		Maps::totalBounds.lon_min = Maps::mercatorX2lon(viewPort.bbox.min.x);
		Maps::totalBounds.lon_max = Maps::mercatorX2lon(viewPort.bbox.max.x);

		ESP_LOGI(TAG, "Total Bounds: Lat Min: %f, Lat Max: %f, Lon Min: %f, Lon Max: %f",
			Maps::totalBounds.lat_min, Maps::totalBounds.lat_max, Maps::totalBounds.lon_min, Maps::totalBounds.lon_max);

		if (Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
			Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &(Maps::wptPosX), &(Maps::wptPosY));
		else
		{
			Maps::wptPosX = -1;
			Maps::wptPosY = -1;
		}

		for (size_t i = 1; i < trackData.size(); ++i)
		{
		if ( trackData[i-1].lon > Maps::totalBounds.lon_min && trackData[i-1].lon < Maps::totalBounds.lon_max &&
			trackData[i-1].lat > Maps::totalBounds.lat_min && trackData[i-1].lat < Maps::totalBounds.lat_max &&
			trackData[i].lon > Maps::totalBounds.lon_min && trackData[i].lon < Maps::totalBounds.lon_max &&
			trackData[i].lat > Maps::totalBounds.lat_min && trackData[i].lat < Maps::totalBounds.lat_max )
		{
			uint16_t x, y, x2, y2;
			Maps::coords2map(trackData[i-1].lat,trackData[i-1].lon,Maps::totalBounds, &x,&y);
			Maps::coords2map(trackData[i].lat,trackData[i].lon,Maps::totalBounds, &x2,&y2);

			map.drawWideLine(x,y,x2,y2,2,TFT_BLUE);                                                                         
		}    
		}
	}
	else
	{
		Maps::isMapFound = false;
		map.fillScreen(TFT_BLACK);
		Maps::showNoMap(map);
		ESP_LOGE(TAG, "Map doesn't exist");
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
	Coord pos;
	pos.lat = lat;
	pos.lng = lon;
	if (abs(pos.lat - Maps::prevLat) > 0.00005 && abs(pos.lng - Maps::prevLon) > 0.00005)
	{
		Maps::point.x = Maps::lon2x(pos.lng);
		Maps::point.y = Maps::lat2y(pos.lat);
		Maps::prevLat = pos.lat;
		Maps::prevLon = pos.lng;
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


	if (Maps::followGps)
		Maps::currentMapTile = Maps::getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, Maps::zoomLevel, 0, 0);
	else
		Maps::currentMapTile = Maps::getMapTile(Maps::currentMapTile.lon, Maps::currentMapTile.lat, Maps::zoomLevel, 0, 0);

	// Detects if tile changes from actual GPS position
	if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 || Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
		Maps::currentMapTile.tilex != Maps::oldMapTile.tilex || Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
	{ 
		Maps::isMapFound = Maps::mapTempSprite.drawPngFile(Maps::currentMapTile.file, Maps::mapTileSize, Maps::mapTileSize);
		
		if (!Maps::isMapFound)
		{
			ESP_LOGE(TAG, "No Map Found!");
			Maps::isMapFound = false;
			Maps::oldMapTile = Maps::currentMapTile;
			strcpy(Maps::oldMapTile.file,Maps::currentMapTile.file);
			Maps::mapTempSprite.fillScreen(TFT_BLACK);
			Maps::showNoMap(Maps::mapTempSprite);
		}
		else
		{
			Maps::oldMapTile = Maps::currentMapTile;
			strcpy(Maps::oldMapTile.file,Maps::currentMapTile.file);

			Maps::totalBounds = Maps::getTileBounds(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, Maps::zoomLevel);

			int8_t startX = -1;
			int8_t startY = -1;

			for (int8_t y = startX; y <= startX + 2 ; y++)
			{
				for (int8_t x = startY; x <= startY + 2; x++)
				{         

					if (x == 0 && y == 0)
						continue;// Skip Center Tile

					Maps::roundMapTile = getMapTile(Maps::currentMapTile.lon, Maps::currentMapTile.lat, Maps::zoomLevel, x, y);

					foundRoundMap = Maps::mapTempSprite.drawPngFile(Maps::roundMapTile.file, (x - startX) * Maps::mapTileSize, (y - startY) * Maps::mapTileSize);
					if (!foundRoundMap)
					{
						Maps::mapTempSprite.fillRect((x - startX) * Maps::mapTileSize, (y - startY) * Maps::mapTileSize, Maps::mapTileSize, Maps::mapTileSize, TFT_BLACK);
						Maps::mapTempSprite.drawPngFile(noMapFile, ((x - startX) * Maps::mapTileSize) + (Maps::mapTileSize / 2) - 50, ((y - startY) * Maps::mapTileSize) + (Maps::mapTileSize / 2) - 50);
						missingMap = true;
					}
					else
					{
						tileBounds currentBounds = Maps::getTileBounds(Maps::roundMapTile.tilex, Maps::roundMapTile.tiley, Maps::zoomLevel);

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
					Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &(Maps::wptPosX), &(Maps::wptPosY));
			}
			else
			{
				Maps::wptPosX = -1;
				Maps::wptPosY = -1;
			}

			Maps::redrawMap = true;

			for (size_t i = 1; i < trackData.size(); i++)
			{
				if ( trackData[i-1].lon > Maps::totalBounds.lon_min && trackData[i-1].lon < Maps::totalBounds.lon_max &&
					trackData[i-1].lat > Maps::totalBounds.lat_min && trackData[i-1].lat < Maps::totalBounds.lat_max &&
					trackData[i].lon > Maps::totalBounds.lon_min && trackData[i].lon < Maps::totalBounds.lon_max &&
					trackData[i].lat > Maps::totalBounds.lat_min && trackData[i].lat < Maps::totalBounds.lat_max )
				{
				uint16_t x, y, x2, y2;
					Maps::coords2map(trackData[i-1].lat,trackData[i-1].lon,Maps::totalBounds, &x,&y);
					Maps::coords2map(trackData[i].lat,trackData[i].lon,Maps::totalBounds, &x2,&y2);

					Maps::mapTempSprite.drawWideLine(x,y,x2,y2,2,TFT_BLUE);                                                                         
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
	Maps::getPosition(gps.gpsData.latitude, gps.gpsData.longitude);
	if (Maps::isPosMoved)
	{
		Maps::mapTileSize = Maps::vectorMapTileSize;
		Maps::zoomLevel = zoom;
		Maps::viewPort.setCenter(Maps::point);
		Maps::getMapBlocks(Maps::viewPort.bbox, Maps::memCache);
		Maps::readVectorMap(Maps::viewPort, Maps::memCache, Maps::mapTempSprite, zoom);
		Maps::isPosMoved = false;
	}
}

/**
 * @brief Display Map
 *
 * @details Displays the map on the screen. 
 */
void Maps::displayMap()
{
	if (Maps::isMapFound)
	{
		uint16_t mapHeading = 0;
	#ifdef ENABLE_COMPASS

		if (mapSet.mapRotationComp)
			mapHeading = compass.getHeading();
		else
			mapHeading = gps.gpsData.heading;
	#else
		mapHeading = gps.gpsData.heading;
	#endif

		Maps::mapTempSprite.pushImage(Maps::wptPosX - 8, Maps::wptPosY - 8, 16, 16, (uint16_t *)waypoint, TFT_BLACK);

		if (Maps::mapTileSize == Maps::renderMapTileSize && Maps::followGps)
		{
			Maps::navArrowPosition = Maps::coord2ScreenPos(gps.gpsData.longitude, gps.gpsData.latitude, Maps::zoomLevel, Maps::renderMapTileSize);
			Maps::mapTempSprite.setPivot(Maps::renderMapTileSize + Maps::navArrowPosition.posX, Maps::renderMapTileSize + Maps::navArrowPosition.posY);
		}
		else if (Maps::mapTileSize == Maps::renderMapTileSize && !Maps::followGps)
		{
			int16_t pivotX = Maps::tileWidth / 2 + Maps::offsetX;
			int16_t pivotY = Maps::tileHeight / 2 + Maps::offsetY;
			Maps::mapTempSprite.setPivot(pivotX, pivotY);
		}
		else if (Maps::mapTileSize == Maps::vectorMapTileSize)
			Maps::mapTempSprite.setPivot(Maps::vectorMapTileSize, Maps::vectorMapTileSize);

		if (Maps::followGps)
			Maps::mapTempSprite.pushRotated(&(Maps::mapSprite), 360 - mapHeading, TFT_TRANSPARENT);
		else
			Maps::mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);
	}
	else
	{
		if (Maps::scrollUpdated && !Maps::followGps)
			Maps::mapTempSprite.pushSprite(&(Maps::mapSprite), 0, 0, TFT_TRANSPARENT);
		else
			Maps::mapTempSprite.pushSprite(&(Maps::mapSprite), 0, 0, TFT_TRANSPARENT);
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

	speedX = (speedX + dx) * inertia;
	speedY = (speedY + dy) * inertia;

	speedX *= friction;
	speedY *= friction;

	if (fabs(speedX) > maxSpeed) speedX = (speedX > 0) ? maxSpeed : -maxSpeed;
	if (fabs(speedY) > maxSpeed) speedY = (speedY > 0) ? maxSpeed : -maxSpeed;
	
	Maps::offsetX += (int16_t)speedX;
	Maps::offsetY += (int16_t)speedY;

	// Maps::offsetX += (int16_t)dx;
	// Maps::offsetY += (int16_t)dy;

	Maps::scrollUpdated = false;
	Maps::followGps = false;

	if (Maps::offsetX <= -Maps::scrollThreshold) 
	{
		Maps::tileX--;  
		Maps::offsetX += Maps::renderMapTileSize;  
		Maps::scrollUpdated = true;
	}
	else if (offsetX >= Maps::scrollThreshold)
	{
		Maps::tileX++;  
		Maps::offsetX -= Maps::renderMapTileSize;
		Maps::scrollUpdated = true;
	}

	if (Maps::offsetY <= -Maps::scrollThreshold)
	{
		Maps::tileY--; 
		Maps::offsetY += Maps::renderMapTileSize;
		Maps::scrollUpdated = true;
	} 
	else if (Maps::offsetY >= Maps::scrollThreshold)
	{
		Maps::tileY++;  
		Maps::offsetY -= Maps::renderMapTileSize;
		Maps::scrollUpdated = true;
	}

	if (Maps::scrollUpdated)
	{
		int8_t deltaTileX = Maps::tileX - Maps::lastTileX;
		int8_t deltaTileY = Maps::tileY - Maps::lastTileY;
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
	int16_t preloadWidth = (dirX != 0) ? renderMapTileSize : renderMapTileSize * 2;
	int16_t preloadHeight = (dirY != 0) ? renderMapTileSize : renderMapTileSize * 2;

	TFT_eSprite preloadSprite = TFT_eSprite(&tft);
	preloadSprite.createSprite(preloadWidth, preloadHeight);

	int16_t startX = tileX + dirX;
	int16_t startY = tileY + dirY;

	for (int8_t i = 0; i < 2; ++i) 
	{
		int16_t tileToLoadX = startX + ((dirX == 0) ? i - 1 : 0);
		int16_t tileToLoadY = startY + ((dirY == 0) ? i - 1 : 0);

		Maps::roundMapTile = Maps::getMapTile(
			Maps::currentMapTile.lon,
			Maps::currentMapTile.lat,
			Maps::zoomLevel,
			tileToLoadX,
			tileToLoadY
		);

		bool foundTile = preloadSprite.drawPngFile(
		Maps::roundMapTile.file,
		(dirX != 0) ? i * renderMapTileSize : 0,
		(dirY != 0) ? i * renderMapTileSize : 0
		);

		if (!foundTile)
		{
			preloadSprite.fillRect(
				(dirX != 0) ? i * renderMapTileSize : 0,
				(dirY != 0) ? i * renderMapTileSize : 0,
				renderMapTileSize,
				renderMapTileSize,
				TFT_LIGHTGREY
			);
		}
	}

	if (dirX != 0)
	{
		mapTempSprite.scroll(dirX * renderMapTileSize, 0);
		mapTempSprite.pushImage(
			(dirX > 0 ? renderMapTileSize * 2 : 0),
			0,
			preloadWidth,
			preloadHeight,
			preloadSprite.frameBuffer(0)
		);
	}
	else if (dirY != 0) 
	{
		mapTempSprite.scroll(0, dirY * renderMapTileSize);
		mapTempSprite.pushImage(
			0,
			(dirY > 0 ? renderMapTileSize * 2 : 0),
			preloadWidth,
			preloadHeight,
			preloadSprite.frameBuffer(0)
		);
	}

	preloadSprite.deleteSprite();
}