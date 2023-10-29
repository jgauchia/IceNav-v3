#ifndef graphics_h_
#define graphics_h_
#include <Arduino.h>
#include <stdint.h>
#include <vector>

const uint16_t WHITE        =   0xFFFF;
const uint16_t BLACK        =   0x0000;
//const uint16_t RED          =   0xFA45;
const uint16_t GREEN        =   0x76EE;
const uint16_t GREENCLEAR   =   0x9F93;
const uint16_t GREENCLEAR2  =   0xCF6E;
const uint16_t BLUE         =   0x227E;
const uint16_t BLUECLEAR    =   0x6D3E;
const uint16_t CYAN         =   0xAA1F;
const uint16_t ORANGE       =   0xFCC2;
const uint16_t GRAY         =   0x94B2;
const uint16_t GRAYCLEAR    =   0xAD55;
const uint16_t GRAYCLEAR2   =   0xD69A;
const uint16_t BROWN        =   0xAB00;
//const uint16_t YELLOW       =   0xFFF1;
const uint16_t YELLOWCLEAR  =   0xFFF5;
const uint16_t BACKGROUND_COLOR = 0xEF5D;

/// @brief Point in 32 bits projected coordinates (x,y) 
struct Point32 {
    Point32(){};
    Point32( int32_t x, int32_t y) : x(x),y(y) {};
    Point32 operator-(const Point32 p){ return Point32( x-p.x, y-p.y);};
    Point32 operator+(const Point32 p){ return Point32( x+p.x, y+p.y);};

    /// @brief Parse char array with the coordinates
    /// @param coords_pair char array like:  11.222,333.44
    Point32( char *coords_pair); // char array like:  11.222,333.44
    
    int32_t x;
    int32_t y;
};


/// @brief Point in 16 bits projected coordinates (x,y) 
struct Point16 {
    Point16(){};
    Point16( int16_t x, int16_t y) : x(x),y(y) {};
    Point16( Point32 p): x(p.x), y(p.y) {};
    // Point16 operator-(const Point16 p){ return Point16( x-p.x, y-p.y);};
    // Point16 operator+(const Point16 p){ return Point16( x+p.x, y+p.y);};
    int16_t x;
    int16_t y;
};


struct Polyline {
    std::vector<Point16> points;
    uint16_t color;
    uint8_t width;
};


struct Polygon {
    std::vector<Point16> points;
    uint16_t color;
};


/// @brief Bounding Box
struct BBox {
    BBox() {};
    /// @brief Bounding Box
    /// @param min top left corner
    /// @param max bottim right corner
    BBox( Point32 min, Point32 max): min(min), max(max) {};
    BBox operator-(const Point32 p){ return BBox( min-p, max-p);};
    bool contains_point(const Point16 p){ return p.x > min.x && p.x < max.x && p.y > min.y && p.y < max.y; }
    Point32 min;
    Point32 max;
};


struct ViewPort {
    void setCenter(Point32 pcenter);
    Point32 center;
    BBox bbox;
};



/////////////////////  Functions  /////////////////////////

struct MapBlock;
struct MemBlocks;
void draw( ViewPort& viewPort, MemBlocks& memblocks);
Point16 toScreenCoords( Point16 p, Point16 screen_center);
std::vector<Point16> clip_polygon( BBox bbox, std::vector<Point16>  points);
void stats( ViewPort& viewPort, MapBlock* mblock);

/// @brief Clips a segment against a bbox and returns the intersection point. 
/// You should know in advance which point in inside and which outside.
/// @param p_in Point of the segment that is in the bbox
/// @param p_out Point of the segment that is out of the bbox
/// @param bbox clipping area boudaries
/// @return 
// Point16 clip_segment( Point16 p_in, Point16 p_out, BBox bbox);

#endif