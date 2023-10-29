#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <StreamUtils.h>
#include <string>
#include "utils/graphics/graphics.h"
#include "utils/maps/maps.h"

const String base_folder = "/mymap/"; // TODO: folder selection

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

MapBlock *read_map_block(String file_name)
{
    MapBlock *mblock = new MapBlock();
    File file = SD.open(file_name + ".fmp");
    if (!file)
    {
        while (true)
            ;
    }
    ReadBufferingStream bufferedFile{file, 1000};

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
