/**
 * @file vector_maps.h
 * @author @aresta
 * @brief  Vector maps draw functions
 * @version 0.1.6
 * @date 2023-06-14
 */

#include <SPI.h>
#include <SD.h>
#include <StreamUtils.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <array>
#include <math.h>
#include <map>

/**
 * @brief Vector Map folder
 *
 */
const String base_folder = "/mymap/"; // TODO: folder selection

/**
 * @brief Vectorized map size
 *
 */
#define MAP_HEIGHT 374
#define MAP_WIDTH 320

bool position_moved = false;

/**
 * @brief Vector map object colours
 *
 */
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;
const uint16_t GREEN = 0x76EE;
const uint16_t GREENCLEAR = 0x9F93;
const uint16_t GREENCLEAR2 = 0xCF6E;
const uint16_t BLUE = 0x227E;
const uint16_t BLUECLEAR = 0x6D3E;
const uint16_t CYAN = 0xAA1F;
const uint16_t ORANGE = 0xFCC2;
const uint16_t GRAY = 0x94B2;
const uint16_t GRAYCLEAR = 0xAD55;
const uint16_t GRAYCLEAR2 = 0xD69A;
const uint16_t BROWN = 0xAB00;
const uint16_t YELLOWCLEAR = 0xFFF5;
const uint16_t BACKGROUND_COLOR = 0xEF5D;

/**
 * @brief Vector maps memory definition
 *
 */
#define MAPBLOCKS_MAX 6                                         // max blocks in memory
#define MAPBLOCK_SIZE_BITS 12                                   // 4096 x 4096 coords (~meters) per block
#define MAPFOLDER_SIZE_BITS 4                                   // 16 x 16 map blocks per folder
const int32_t mapblock_mask = pow(2, MAPBLOCK_SIZE_BITS) - 1;   // ...00000000111111111111
const int32_t mapfolder_mask = pow(2, MAPFOLDER_SIZE_BITS) - 1; // ...00001111

#define DEG2RAD(a) ((a) / (180 / M_PI))
#define RAD2DEG(a) ((a) * (180 / M_PI))
#define EARTH_RADIUS 6378137
double lat2y(double lat) { return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS; }
double lon2x(double lon) { return DEG2RAD(lon) * EARTH_RADIUS; }

/**
 * @brief Point in 32 bits projected coordinates (x,y)
 *
 */
struct Point32
{
    Point32(){};
    Point32(int32_t x, int32_t y) : x(x), y(y){};
    Point32 operator-(const Point32 p) { return Point32(x - p.x, y - p.y); };
    Point32 operator+(const Point32 p) { return Point32(x + p.x, y + p.y); };

    /// @brief Parse char array with the coordinates
    /// @param coords_pair char array like:  11.222,333.44
    Point32(char *coords_pair); // char array like:  11.222,333.44

    int32_t x;
    int32_t y;
};

/**
 * @brief Point in 16 bits projected coordinates (x,y)
 *
 */
struct Point16
{
    Point16(){};
    Point16(int16_t x, int16_t y) : x(x), y(y){};
    Point16(Point32 p) : x(p.x), y(p.y){};
    // Point16 operator-(const Point16 p){ return Point16( x-p.x, y-p.y);};
    // Point16 operator+(const Point16 p){ return Point16( x+p.x, y+p.y);};
    int16_t x;
    int16_t y;
};

/**
 * @brief Polyline
 *
 */
struct Polyline
{
    std::vector<Point16> points;
    uint16_t color;
    uint8_t width;
};

/**
 * @brief Poligon
 *
 */
struct Polygon
{
    std::vector<Point16> points;
    uint16_t color;
};

/**
 * @brief Bounding Box
 *
 */
struct BBox
{
    BBox(){};
    /// @brief Bounding Box
    /// @param min top left corner
    /// @param max bottim right corner
    BBox(Point32 min, Point32 max) : min(min), max(max){};
    BBox operator-(const Point32 p) { return BBox(min - p, max - p); };
    bool contains_point(const Point16 p) { return p.x > min.x && p.x < max.x && p.y > min.y && p.y < max.y; }
    Point32 min;
    Point32 max;
};

std::vector<Point16> clip_polygon(BBox bbox, std::vector<Point16> points)
{
    std::vector<Point16> clipped;
    int16_t dx, dy, bbpx;
    for (int i = 0; i < (points.size() - 1); i++)
    {
        Point16 p1 = points[i];
        Point16 p2 = points[i + 1];
        // cut vert left side
        if (p1.x < bbox.min.x && p2.x > bbox.min.x)
        {
            dx = p2.x - p1.x;
            dy = abs(p2.y - p1.y);
            bbpx = bbox.min.x - p1.x;
            assert(dx > 0);
            assert(dy > 0);
            assert(bbpx > 0);
            p1.y = double(bbpx * dy) / dx;
            p1.x = bbox.min.x;
        }

        if (p1.x > bbox.min.x && p2.x < bbox.min.x)
        {
        }
    }
    return clipped;
}

Point32::Point32(char *coords_pair)
{
    char *next;
    x = round(strtod(coords_pair, &next)); // 1st coord
    y = round(strtod(++next, NULL));       // 2nd coord
}

/**
 * @brief Vector map viewport
 *
 */
struct ViewPort
{
    void setCenter(Point32 pcenter);
    Point32 center;
    BBox bbox;
};

/**
 * @brief Map square area of aprox 4096 meters side. Correspond to one single map file.
 *
 */
struct MapBlock
{
    Point32 offset;
    BBox bbox;
    bool inView = false;
    std::vector<Polyline> polylines;
    std::vector<Polygon> polygons;
};

/**
 * @brief MapBlocks stored in memory
 *
 */
struct MemBlocks
{
    std::map<String, u_int16_t> blocks_map; // block offset -> block index
    std::array<MapBlock *, MAPBLOCKS_MAX> blocks;
};

/**
 * @brief Set center coordinates of viewport
 *
 * @param pcenter
 */
void ViewPort::setCenter(Point32 pcenter)
{
    center = pcenter;
    bbox.min.x = pcenter.x - MAP_WIDTH * zoom / 2;
    bbox.min.y = pcenter.y - MAP_HEIGHT * zoom / 2;
    bbox.max.x = pcenter.x + MAP_WIDTH * zoom / 2;
    bbox.max.y = pcenter.y + MAP_HEIGHT * zoom / 2;
}

/**
 * @brief Points to screen coordinates
 *
 * @param p
 * @param screen_center
 * @return Point16
 */
Point16 toScreenCoords(Point16 p, Point16 screen_center)
{
    return Point16(
        ((p.x - screen_center.x) / zoom) + MAP_WIDTH / 2,
        ((p.y - screen_center.y) / zoom) + MAP_HEIGHT / 2);
}

/**
 * @brief Point in geografic (lat,lon) coordinates and other gps data
 *
 */
struct Coord
{
    Point32 getPoint32();
    double lat = 0;
    double lng = 0;
    int16_t altitude = 0;
    int16_t direction = 0;
    int16_t satellites = 0;
    bool isValid = false;
    bool isUpdated = false;
};

/**
 * @brief Vector file map memory blocks
 * 
 */
MemBlocks memBlocks;

/**
 * @brief Vector map viewport
 * 
 */
ViewPort viewPort;

/**
 * @brief Vector map GPS position point
 * 
 */
Point32 point = viewPort.center;

/**
 * @brief Get vector map Position from GPS position and check if is moved
 * 
 * @param lat 
 * @param lon 
 */
double prev_lat = 0, prev_lng = 0;
void get_position(double lat, double lon)
{
    Coord pos;
    pos.lat = lat;
    pos.lng = lon;

    if (abs(pos.lat - prev_lat) > 0.00005 &&
        abs(pos.lng - prev_lng) > 0.00005)
    {
      point.x = lon2x(pos.lng);
      point.y = lat2y(pos.lat);
      prev_lat = pos.lat;
      prev_lng = pos.lng;
      position_moved = true;
      refresh_map = false;
    }
}

/**
 * @brief Parse vector file to coords
 *
 * @param file
 * @param points
 */
void parse_coords(ReadBufferingStream &file, std::vector<Point16> &points)
{
    char coord[20];
    int i, c;
    while (true)
    {
        i = 0;
        c = file.read();
        if (c == '\n')
            break;
        while (c >= 0 && c != ';' && i < 20)
        {
            coord[i++] = (char)c;
            c = file.read();
        }
        coord[i] = '\0';
        points.push_back(Point16(coord));
    }
    points.shrink_to_fit();
}

/**
 * @brief Read vector map file to memory block
 *
 * @param file_name
 * @return MapBlock*
 */
MapBlock *read_map_block(String file_name)
{
    MapBlock *mblock = new MapBlock();
    File file = SD.open(file_name + ".fmp");
    if (!file)
    {
        while (true)
            ;
    }
    ReadBufferingStream bufferedFile{file, 1024};

    // read polygons
    String feature_type = bufferedFile.readStringUntil(':');
    if (feature_type != "Polygons")
        log_e("Map error. Expected Polygons instead of: %s", feature_type);
    u_int32_t count = bufferedFile.readStringUntil('\n').toInt();

    if (count <= 0)
    {
        while (true)
            ;
    }
    int line = 5;
    int total_points = 0;
    Polygon polygon;
    while (count > 0 && bufferedFile.available())
    {
        polygon.color = std::stoul(bufferedFile.readStringUntil('\n').c_str(), nullptr, 16);
        line++;
        polygon.points.clear();
        parse_coords(bufferedFile, polygon.points);
        line++;
        mblock->polygons.push_back(polygon);
        total_points += polygon.points.size();
        count--;
    }
    if (count != 0)
    {
        while (true)
            ;
    }
    mblock->polygons.shrink_to_fit();

    // read lines
    feature_type = bufferedFile.readStringUntil(':');
    if (feature_type != "Polylines")
        log_e("Map error. Expected Polylines instead of: %s", feature_type);
    count = bufferedFile.readStringUntil('\n').toInt();

    log_v("%d", count);
    if (count <= 0)
    {
        while (true)
            ;
    }
    Polyline polyline;
    while (count > 0 && bufferedFile.available())
    {
        polyline.color = std::stoul(bufferedFile.readStringUntil('\n').c_str(), nullptr, 16);
        line++;
        polyline.width = bufferedFile.readStringUntil('\n').toInt() ?: 1;
        line++;
        polyline.points.clear();
        parse_coords(bufferedFile, polyline.points);
        line++;
        mblock->polylines.push_back(polyline);
        total_points += polyline.points.size();
        count--;
    }
    if (count != 0)
    {
        while (true)
            ;
    }
    mblock->polylines.shrink_to_fit();
    file.close();
    return mblock;
}

/**
 * @brief Get bounding objects in memory block
 *
 * @param memBlocks
 * @param bbox
 */
void get_map_blocks(MemBlocks &memBlocks, BBox &bbox)
{
    for (MapBlock *block : memBlocks.blocks)
    {
        log_d("Block: %p", block);
        if (block)
            block->inView = false;
    }
    for (Point32 point : {bbox.min, bbox.max, Point32(bbox.min.x, bbox.max.y), Point32(bbox.max.x, bbox.min.y)})
    {
        int32_t min_x = point.x & (~mapblock_mask);
        int32_t min_y = point.y & (~mapblock_mask);
        int32_t block_x = (min_x >> MAPBLOCK_SIZE_BITS) & mapfolder_mask;
        int32_t block_y = (min_y >> MAPBLOCK_SIZE_BITS) & mapfolder_mask;
        int32_t folder_name_x = min_x >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
        int32_t folder_name_y = min_y >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
        String file_name = base_folder + folder_name_x + "_" + folder_name_y + "/" + block_x + "_" + block_y; //  /maps/123_456/777_888

        log_v("%d %d", block_x, block_y);

        if (memBlocks.blocks_map.count(file_name))
        { // if contains
            assert(memBlocks.blocks[memBlocks.blocks_map[file_name]]);
            memBlocks.blocks[memBlocks.blocks_map[file_name]]->inView = true;
            log_d("Block in memory: %p", memBlocks.blocks[memBlocks.blocks_map[file_name]]);
        }
        else
        {
            MapBlock *new_block = read_map_block(file_name);
            assert(new_block);
            new_block->inView = true;
            new_block->offset = Point32(min_x, min_y);
            int i = 0;
            while (i < MAPBLOCKS_MAX)
            {
                if (memBlocks.blocks[i] == NULL)
                {
                    memBlocks.blocks[i] = new_block;
                    memBlocks.blocks_map[file_name] = i;
                    break;
                }
                i++;
            }
            assert(i < MAPBLOCKS_MAX); // TODO: handle replacement of block when array is full. Free removed blocks.
            log_d("Block readed from SD card: %p", new_block);
            log_d("FreeHeap: %i", esp_get_free_heap_size());
        }
    }
}

/**
 * @brief Fill polygon routine
 *
 * @param points
 * @param color
 */
void fill_polygon(std::vector<Point16> points, int color, TFT_eSprite &map) // scanline fill algorithm
{
    int16_t maxy = INT16_MIN, miny = INT16_MAX;

    for (Point16 p : points)
    { // TODO: precalculate at map file creation
        maxy = max(maxy, p.y);
        miny = min(miny, p.y);
    }
    if (maxy > MAP_HEIGHT)
        maxy = MAP_HEIGHT;
    if (miny < 0)
        miny = 0;
    assert(miny < maxy);

    int16_t nodeX[points.size()], pixelY;

    //  Loop through the rows of the image.
    for (pixelY = miny; pixelY < maxy; pixelY++)
    {
        //  Build a list of nodes.
        int16_t nodes = 0;
        for (int i = 0; i < (points.size() - 1); i++)
        {
            if ((points[i].y < pixelY && points[i + 1].y >= pixelY) ||
                (points[i].y >= pixelY && points[i + 1].y < pixelY))
            {
                nodeX[nodes++] =
                    points[i].x + double(pixelY - points[i].y) / double(points[i + 1].y - points[i].y) *
                                      double(points[i + 1].x - points[i].x);
            }
        }
        assert(nodes < points.size());

        //  Sort the nodes, via a simple “Bubble” sort.
        int16_t i = 0, swap;
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
            {
                i++;
            }
        }

        //  Fill the pixels between node pairs.
        for (i = 0; i <= nodes - 2; i += 2)
        {
            if (nodeX[i] >= MAP_WIDTH)
                break;
            if (nodeX[i + 1] <= 0)
                continue;
            if (nodeX[i] < 0)
                nodeX[i] = 0;
            if (nodeX[i + 1] > MAP_WIDTH)
                nodeX[i + 1] = MAP_WIDTH;
            map.drawLine(nodeX[i], MAP_HEIGHT - pixelY, nodeX[i + 1], MAP_HEIGHT - pixelY, color);
        }
    }
}

/**
 * @brief Generate vectorized map
 *
 * @param viewPort
 * @param memblocks
 * @param map -> Map Sprite
 */
void generate_vector_map(ViewPort &viewPort, MemBlocks &memblocks, TFT_eSprite &map)
{
    map.fillScreen(BACKGROUND_COLOR);

    std::vector<Polygon> polygons_to_draw;
    std::vector<Polyline> lines_to_draw;
    for (MapBlock *mblock : memblocks.blocks)
    {
        if (!mblock || !mblock->inView)
            continue;
        Point16 screen_center_mc = viewPort.center - mblock->offset; // screen center with features coordinates
        BBox screen_bbox_mc = viewPort.bbox - mblock->offset;        // screen boundaries with features coordinates

        ////// Polygons
        for (Polygon polygon : mblock->polygons)
        {
            if (polygon.color == YELLOW)
                log_w("Polygon type unknown");
            Polygon new_polygon;
            bool hit = false;
            for (Point16 p : polygon.points)
            {
                if (screen_bbox_mc.contains_point(p))
                    hit = true;
                new_polygon.points.push_back(toScreenCoords(p, screen_center_mc));
            }
            if (hit)
            {
                new_polygon.color = polygon.color;
                polygons_to_draw.push_back(new_polygon);
            }
        }

        ////// Lines
        for (Polyline line : mblock->polylines)
        {
            Polyline new_line;
            new_line.color = line.color;
            new_line.width = line.width;
            bool prev_in_screen = false;
            for (int i = 0; i < (line.points.size()); i++)
            {
                bool curr_in_screen = screen_bbox_mc.contains_point(line.points[i]);
                if (!prev_in_screen && !curr_in_screen)
                { // TODO: clip, the segment could still intersect the screen area!
                    prev_in_screen = false;
                    continue;
                }
                if (prev_in_screen && !curr_in_screen)
                { // split polyline: end and start new polyline. Driver does the clipping of the segment.
                    new_line.points.push_back(toScreenCoords(line.points[i], screen_center_mc));
                    lines_to_draw.push_back(new_line);
                    new_line.points.clear();
                    prev_in_screen = false;
                    continue;
                }
                if (!prev_in_screen && curr_in_screen && i > 0)
                { // reenter screen.  Driver does the clipping
                    new_line.points.push_back(toScreenCoords(line.points[i - 1], screen_center_mc));
                }
                new_line.points.push_back(toScreenCoords(line.points[i], screen_center_mc));
                prev_in_screen = curr_in_screen;
            }
            assert(new_line.points.size() != 1);
            if (new_line.points.size() >= 2)
            {
                lines_to_draw.push_back(new_line);
            }
        }
        // map.fillScreen(BACKGROUND_COLOR);
    }

    for (Polygon pol : polygons_to_draw)
    {
        fill_polygon(pol.points, pol.color, map);
    }
    for (Polyline line : lines_to_draw)
    {
        for (int i = 0; i < (line.points.size() - 1); i++)
        {
            if (line.points[i].x < 0 || line.points[i + 1].x < 0 ||
                line.points[i].x > MAP_WIDTH || line.points[i + 1].x > MAP_WIDTH ||
                line.points[i].y < 0 || line.points[i].y > MAP_HEIGHT ||
                line.points[i + 1].y < 0 || line.points[i + 1].y > MAP_HEIGHT)
            {
                log_d("Error: point out of screen: %i, %i, %i, %i", line.points[i].x, line.points[i].y, line.points[i + 1].x, line.points[i + 1].y);
            }

            // map.drawWideLine(
            map.drawLine(
                line.points[i].x, MAP_HEIGHT - line.points[i].y,
                line.points[i + 1].x, MAP_HEIGHT - line.points[i + 1].y,
                // line.width/zoom ?: 1, line.color, line.color);
                line.color);
        }
    }

    map.fillTriangle(
        MAP_WIDTH / 2 - 4, MAP_HEIGHT / 2 + 5,
        MAP_WIDTH / 2 + 4, MAP_HEIGHT / 2 + 5,
        MAP_WIDTH / 2, MAP_HEIGHT / 2 - 6,
        RED);
    log_v("Draw done!");
}