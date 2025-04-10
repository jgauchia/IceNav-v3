/**
 * @file maps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @author @aresta - https://github.com/aresta/ESP32_GPS - Vector Maps
 * @brief  Maps draw class
 * @version 0.2.0
 * @date 2025-04
 */

#include "maps.hpp"

extern Compass compass;
extern Gps gps;
extern Storage storage;

const char* TAG PROGMEM = "Maps";

extern Point16::Point16(char *coordsPair)
{
  char *next;
  x = (int16_t)round(strtod(coordsPair, &next)); // 1st coord // TODO: change by strtol and test
  y = (int16_t)round(strtod(++next, NULL));      // 2nd coord
}

bool BBox::containsPoint(const Point32 p) { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }

bool BBox::intersects(BBox b)
{
  if (b.min.x > max.x || b.max.x < min.x || b.min.y > max.y || b.max.y < min.y)
    return false;
  return true;
}

/**
 * @brief Map Class constructor
 *
 */
Maps::Maps() {}

// Render Map Private section

/**
 * @brief Get pixel X position from OpenStreetMap Render map longitude
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @param tileSize -> tile size
 * @return X position
 */
uint16_t Maps::lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize)
{
  return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Get pixel Y position from OpenStreetMap Render map latitude
 *
 * @param f_lat -> latitude
 * @param zoom -> zoom
 * @param tileSize -> tile size
 * @return Y position
 */
uint16_t Maps::lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize)
{
  return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Get TileX for OpenStreetMap files
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return X value (folder)
 */
uint32_t Maps::lon2tilex(double f_lon, uint8_t zoom)
{
  return (uint32_t)(floor((f_lon + 180.0) / 360.0 * pow(2.0, zoom)));
}

/**
 * @brief Get TileY for OpenStreetMap files
 *
 * @param f_lat -> latitude
 * @param zoom  -> zoom
 * @return Y value (file)
 */
uint32_t Maps::lat2tiley(double f_lat, uint8_t zoom)
{
  return (uint32_t)(floor((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, zoom)));
}

/**
 * @brief Get Longitude from OpenStreetMap files
 *
 * @param tileX -> tile X
 * @param zoom  -> zoom
 * @return longitude
 */
double Maps::tilex2lon(uint32_t tileX, uint8_t zoom)
{
  return tileX / pow(2.0, zoom) * 360.0 - 180.0;
}

/**
 * @brief Get Latitude from OpenStreetMap files
 *
 * @param tileX -> tile Y
 * @param zoom  -> zoom
 * @return latitude
 */
double Maps::tiley2lat(uint32_t tileY, uint8_t zoom)
{
  double n = M_PI - 2.0 * M_PI * tileY / pow(2.0, zoom);
  return 180.0 / M_PI * atan(sinh(n));
}

/**
 * @brief Get the map tile structure from GPS Coordinates
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> zoom level
 * @param offsetX -> Tile Offset X
 * @param offsetY -> Tile Offset Y
 * @return MapTile -> Map Tile structure
 */
Maps::MapTile Maps::getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY)
{
  static char tileFile[40] = "";
  uint32_t x = Maps::lon2tilex(lon, zoomLevel) + offsetX;
  uint32_t y = Maps::lat2tiley(lat, zoomLevel) + offsetY;

  sprintf(tileFile, mapRenderFolder, zoomLevel, x, y);
  MapTile data;
  data.file = tileFile;
  data.tilex = x;
  data.tiley = y;
  data.zoom = zoomLevel;
  return data;
}

// Vector Map Private section

/**
 * @brief Get pixel Y position from OpenStreetMap Vector map latitude
 *
 * @param lat -> latitude
 * @return Y position
 */
double Maps::lat2y(double lat)
{
  return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
}

/**
 * @brief Get pixel X position from OpenStreetMap Vector map longitude
 *
 * @param lon -> longitude
 * @return X position
 */
double Maps::lon2x(double lon)
{
  return DEG2RAD(lon) * EARTH_RADIUS;
}

/**
 * @brief Get longitude from X position in Vector Map (Mercator projection)
 *
 * @param x -> X position
 * @return longitude
 */
double Maps::mercatorX2lon(double x)
{
  return (x / EARTH_RADIUS) * (180.0 / M_PI);
}

/**
 * @brief Get latitude from Y position in Vector Map (Mercator projection)
 *
 * @param y -> Y position
 * @return latitude
 */
double Maps::mercatorY2lat(double y)
{
  return (atan(sinh(y / EARTH_RADIUS))) * (180.0 / M_PI);
}

/**
 * @brief Points to screen coordinates
 *
 * @param pxy
 * @param screenCenterxy
 * @return int16_t
 */
int16_t Maps::toScreenCoord(const int32_t pxy, const int32_t screenCenterxy)
{
  return round((double)(pxy - screenCenterxy) / zoom) + (double)Maps::tileWidth / 2;
}

/**
 * @brief Returns int16 or 0 if empty
 *
 * @param file
 * @return int16_t
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
 * @param file
 * @param terminator
 * @param str
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
 * @param file
 * @param points
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
 * @param str
 * @return BBox
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
 * @param fileName
 * @return MapBlock*
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
 * @param points
 * @param color
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
 * @param memBlocks
 * @param bbox
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
 * @param viewPort
 * @param memblocks
 * @param map -> Map Sprite
 * @param zoom -> Zoom Level
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
 * @brief Get vector map Position from GPS position and check if is moved
 *
 * @param lat
 * @param lon
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
 * @param tileX -> tile X
 * @param tileY -> tile Y
 * @param zoom  -> zoom
 * @return tileBounds -> min and max longitude and latitude
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
 * @param lat -> latitude
 * @param lon -> longitude
 * @param bound -> map bounds
 * @return true/false
 */
bool Maps::isCoordInBounds(double lat, double lon, tileBounds bound)
{
  return (lat >= Maps::totalBounds.lat_min && lat <= Maps::totalBounds.lat_max && lon >= Maps::totalBounds.lon_min && lon <= Maps::totalBounds.lon_max);
}

/**
 * @brief Convert GPS Coordinates to screen position (with offsets)
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> Zoom level
 * @param tileSize -> tile size
 * @return ScreenCoord -> Screen position
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
 * @param lat -> latitude
 * @param lon -> longitude
 * @param bound -> map bounds
 * @param pixelX -> X position on map
 * @param pixelY -> Y position on map
 */
void Maps::coords2map(double lat, double lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY)
{
  double lon_ratio = (lon - bound.lon_min) / (bound.lon_max - bound.lon_min);
  double lat_ratio = (bound.lat_max - lat) / (bound.lat_max - bound.lat_min);

  *pixelX = (int)(lon_ratio * Maps::tileWidth);
  *pixelY = (int)(lat_ratio * Maps::tileHeight);

  ESP_LOGI(TAG, "Pixel X: %d, Pixel Y: %d\n", *pixelX, *pixelY);
}

/**
 * @brief Load No Map Image
 *
 */
void Maps::showNoMap(TFT_eSprite &map)
{
  map.drawPngFile(noMapFile, (Maps::mapScrWidth / 2) - 50, (Maps::mapScrHeight / 2) - 50);
  map.drawCenterString("NO MAP FOUND", (Maps::mapScrWidth / 2), (Maps::mapScrHeight >> 1) + 65, &fonts::DejaVu18);
}

/**
 * @brief Draw map widgets
 *
 */
void Maps::drawMapWidgets(MAP mapSettings)
{
  Maps::mapSprite.setTextColor(TFT_WHITE, TFT_WHITE);

  uint16_t mapHeading = 0;
#ifdef ENABLE_COMPASS
  if (mapSettings.mapRotationComp)
    mapHeading = compass.getHeading();
  else
    mapHeading = gps.gpsData.heading;
#else
  mapHeading = gps.gpsData.heading;
#endif

  if (mapSettings.showMapCompass)
  {
    Maps::mapSprite.fillRectAlpha(Maps::mapScrWidth - 48, 0, 48, 48, 95, TFT_BLACK);
    if (mapSettings.compassRotation)
      Maps::mapSprite.pushImageRotateZoom(Maps::mapScrWidth - 24, 24, 24, 24, 360 - mapHeading, 1, 1, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
    else
      Maps::mapSprite.pushImage(Maps::mapScrWidth - 48, 0, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
  }

  uint16_t mapHeight = 0;
  if (mapSettings.mapFullScreen)
    mapHeight = Maps::mapScrFull;
  else
    mapHeight = Maps::mapScrHeight;

  uint8_t toolBarOffset = 0;
  uint8_t toolBarSpace = 0;
#ifdef LARGE_SCREEN
  toolBarOffset = 100;
  toolBarSpace = 60;
#endif
#ifndef LARGE_SCREEN
  toolBarOffset = 80;
  toolBarSpace = 50;
#endif

  if (showToolBar)
  {
    if (mapSettings.mapFullScreen)
      Maps::mapSprite.pushImage(10, mapHeight - toolBarOffset, 48, 48, (uint16_t *)collapse, TFT_BLACK);
    else
      Maps::mapSprite.pushImage(10, mapHeight - toolBarOffset, 48, 48, (uint16_t *)expand, TFT_BLACK);

      Maps::mapSprite.fillRectAlpha(10, mapHeight - toolBarOffset, 48, 48, 50, TFT_BLACK);

    Maps::mapSprite.pushImage(10, mapHeight - (toolBarOffset + toolBarSpace), 48, 48, (uint16_t *)zoomout, TFT_BLACK);
    Maps::mapSprite.fillRectAlpha(10, mapHeight - (toolBarOffset + toolBarSpace), 48, 48, 50, TFT_BLACK);

    Maps::mapSprite.pushImage(10, mapHeight - (toolBarOffset + (2 * toolBarSpace)), 48, 48, (uint16_t *)zoomin, TFT_BLACK);
    Maps::mapSprite.fillRectAlpha(10, mapHeight - (toolBarOffset + (2 * toolBarSpace)), 48, 48, 50, TFT_BLACK);
  }

  Maps::mapSprite.fillRectAlpha(0, 0, 50, 32, 95, TFT_BLACK);
  Maps::mapSprite.pushImage(0, 4, 24, 24, (uint16_t *)zoom_ico, TFT_BLACK);
  Maps::mapSprite.drawNumber(zoom, 26, 8, &fonts::FreeSansBold9pt7b);

  if (mapSettings.showMapSpeed)
  {
    Maps::mapSprite.fillRectAlpha(0, mapHeight - 32, 70, 32, 95, TFT_BLACK);
    Maps::mapSprite.pushImage(0, mapHeight - 28, 24, 24, (uint16_t *)speed_ico, TFT_BLACK);
    Maps::mapSprite.drawNumber(gps.gpsData.speed, 26, mapHeight - 24, &fonts::FreeSansBold9pt7b);
  }

  if (!mapSettings.vectorMap)
    if (mapSettings.showMapScale)
    {
      Maps::mapSprite.fillRectAlpha(Maps::mapScrWidth - 70, mapHeight - 32, 70, Maps::mapScrWidth - 75, 95, TFT_BLACK);
      Maps::mapSprite.setTextSize(1);
      Maps::mapSprite.drawFastHLine(Maps::mapScrWidth - 65, mapHeight - 14, 60);
      Maps::mapSprite.drawFastVLine(Maps::mapScrWidth - 65, mapHeight - 19, 10);
      Maps::mapSprite.drawFastVLine(Maps::mapScrWidth - 5, mapHeight - 19, 10);
      Maps::mapSprite.drawCenterString(map_scale[zoom], Maps::mapScrWidth - 35, mapHeight - 24);
    }
}

/**
 * @brief Set center coordinates of viewport
 *
 * @param pcenter
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
 * @param mapHeight  -> Screen map size height
 * @param mapWidth   -> Screen map size width
 * @param mapFull    -> Full Screen map size
 */
void Maps::initMap(uint16_t mapHeight, uint16_t mapWidth, uint16_t mapFull)
{
  Maps::mapScrHeight = mapHeight;
  Maps::mapScrWidth = mapWidth;
  Maps::mapScrFull = mapFull;

  // Reserve PSRAM for buffer map
  Maps::mapTempSprite.deleteSprite();
  Maps::mapTempSprite.createSprite(tileHeight, tileWidth);

  Maps::oldMapTile = {(char *)"", 0, 0, 0};     // Old Map tile coordinates and zoom
  Maps::currentMapTile = {(char *)"", 0, 0, 0}; // Current Map tile coordinates and zoom
  Maps::roundMapTile = {(char *)"", 0, 0, 0};   // Boundaries Map tiles
  Maps::navArrowPosition = {0, 0};              // Map Arrow position

  Maps::totalBounds = {90.0, -90.0, 180.0, -180.0};
}

/**
 * @brief Delete map screen and release PSRAM
 *
 */
void Maps::deleteMapScrSprites()
{
  Maps::arrowSprite.deleteSprite();
  Maps::mapSprite.deleteSprite();
}

/**
 * @brief Create map screen 
 *
 */
void Maps::createMapScrSprites()
{
  // Map Sprite
  if (!mapSet.mapFullScreen)
    Maps::mapSprite.createSprite(Maps::mapScrWidth, Maps::mapScrHeight);
  else
    Maps::mapSprite.createSprite(Maps::mapScrWidth, Maps::mapScrFull);
  // Arrow Sprite
  Maps::arrowSprite.createSprite(16, 16);
  Maps::arrowSprite.setColorDepth(16);
  Maps::arrowSprite.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
}

/**
 * @brief Generate render map
 *
 * @param zoom -> Zoom Level
 */
void Maps::generateRenderMap(uint8_t zoom)
{
  Maps::mapTileSize = Maps::renderMapTileSize;
  Maps::zoomLevel = zoom;

  Maps::currentMapTile = getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, 0, 0);

  bool foundRoundMap = false;
  bool missingMap = false;

  // Detects if tile changes from actual GPS position
  if (strcmp(Maps::currentMapTile.file, Maps::oldMapTile.file) != 0 || Maps::currentMapTile.zoom != Maps::oldMapTile.zoom ||
      Maps::currentMapTile.tilex != Maps::oldMapTile.tilex || Maps::currentMapTile.tiley != Maps::oldMapTile.tiley)
  {
    Maps::isMapFound = Maps::mapTempSprite.drawPngFile(Maps::currentMapTile.file, Maps::mapTileSize, Maps::mapTileSize);

    if (!Maps::isMapFound)
    {
      ESP_LOGE(TAG, "No Map Found!");
      Maps::isMapFound = false;
      Maps::oldMapTile.file = Maps::currentMapTile.file;
      Maps::oldMapTile.zoom = Maps::currentMapTile.zoom;
      Maps::oldMapTile.tilex = Maps::currentMapTile.tilex;
      Maps::oldMapTile.tiley = Maps::currentMapTile.tiley;
      Maps::mapTempSprite.fillScreen(TFT_BLACK);
      Maps::showNoMap(Maps::mapTempSprite);
    }
    else
    {
      ESP_LOGI(TAG, "Map Found!");
      Maps::oldMapTile.file = Maps::currentMapTile.file;

      Maps::totalBounds = Maps::getTileBounds(Maps::currentMapTile.tilex, Maps::currentMapTile.tiley, zoom);

      static const uint8_t centerX = 0;
      static const uint8_t centerY = 0;
      int8_t startX = centerX - 1;
      int8_t startY = centerY - 1;

      for (int y = startY; y <= startY + 2; y++)
      {
        for (int x = startX; x <= startX + 2; x++)
        {
          if (x == centerX && y == centerY)
            continue;// Skip Center Tile
           
          Maps::roundMapTile = getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, x, y);

          foundRoundMap = Maps::mapTempSprite.drawPngFile(Maps::roundMapTile.file, (x - startX) * Maps::mapTileSize, (y - startY) * Maps::mapTileSize);
          if (!foundRoundMap)
          {
            Maps::mapTempSprite.fillRect((x - startX) * Maps::mapTileSize, (y - startY) * Maps::mapTileSize, Maps::mapTileSize, Maps::mapTileSize, TFT_BLACK);
            Maps::mapTempSprite.drawPngFile(noMapFile, ((x - startX) * Maps::mapTileSize) + (Maps::mapTileSize / 2) - 50, ((y - startY) * Maps::mapTileSize) + (Maps::mapTileSize / 2) - 50);
            missingMap = true;
          }
          else
          {
            tileBounds currentBounds = Maps::getTileBounds(Maps::roundMapTile.tilex, Maps::roundMapTile.tiley, zoom);

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
        ESP_LOGI(TAG, "Total Bounds: Lat Min: %f, Lat Max: %f, Lon Min: %f, Lon Max: %f",
              Maps::totalBounds.lat_min, Maps::totalBounds.lat_max, Maps::totalBounds.lon_min, Maps::totalBounds.lon_max);

        if (Maps::isCoordInBounds(Maps::destLat, Maps::destLon, Maps::totalBounds))
          Maps::coords2map(Maps::destLat, Maps::destLon, Maps::totalBounds, &(Maps::wptPosX), &(Maps::wptPosY));
      }
      else
      {
        Maps::wptPosX = -1;
        Maps::wptPosY = -1;
      }
      Maps::oldMapTile.zoom = Maps::currentMapTile.zoom;
      Maps::oldMapTile.tilex = Maps::currentMapTile.tilex;
      Maps::oldMapTile.tiley = Maps::currentMapTile.tiley;

      Maps::redrawMap = true;
    }

    ESP_LOGI(TAG, "TILE: %s", Maps::oldMapTile.file);
  }
}

/**
 * @brief Generate vector map
 *
 * @param zoom -> Zoom Level
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
 */
void Maps::displayMap()
{
  if (!mapSet.mapFullScreen)
    Maps::mapSprite.pushSprite(0, 27);
  else
    Maps::mapSprite.pushSprite(0, 0);

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

    if (Maps::mapTileSize == Maps::renderMapTileSize)
    {
      Maps::navArrowPosition = Maps::coord2ScreenPos(gps.gpsData.longitude, gps.gpsData.latitude, Maps::zoomLevel, Maps::renderMapTileSize);
      Maps::mapTempSprite.setPivot(Maps::renderMapTileSize + Maps::navArrowPosition.posX, Maps::renderMapTileSize + Maps::navArrowPosition.posY);
    }
    if (Maps::mapTileSize == Maps::vectorMapTileSize)
      Maps::mapTempSprite.setPivot(Maps::vectorMapTileSize, Maps::vectorMapTileSize);

    Maps::mapTempSprite.pushRotated(&(Maps::mapSprite), 360 - mapHeading, TFT_TRANSPARENT);
    //Maps::mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);

    Maps::arrowSprite.pushRotated(&(Maps::mapSprite), 0, TFT_BLACK);
    Maps::drawMapWidgets(mapSet);
  }
  else
    Maps::mapTempSprite.pushSprite(&(Maps::mapSprite), 0, 0, TFT_TRANSPARENT);
}

/**
 * @brief Set Waypoint coords in Map
 *
 * @param wptLat -> Waypoint Latitude
 * @param wptLon -> Waypoint Longitude
 */
void Maps::setWaypoint(double wptLat, double wptLon)
{
  Maps::destLat = wptLat;
  Maps::destLon = wptLon;
  Maps::oldMapTile = {(char *)"", 0, 0, 0};
  Maps::isPosMoved = true;
}