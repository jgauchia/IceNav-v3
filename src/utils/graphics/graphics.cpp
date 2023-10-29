#include <Arduino.h>
//#include <TFT_eSPI.h>
#include <stdint.h>
#include "utils/graphics/graphics.h"
//#include "../conf.h"


int pixel_size = 2;
 
void ViewPort::setCenter(Point32 pcenter) {
    center = pcenter;
    bbox.min.x = pcenter.x - TFT_WIDTH  * pixel_size / 2;
    bbox.min.y = pcenter.y - TFT_HEIGHT * pixel_size / 2;
    bbox.max.x = pcenter.x + TFT_WIDTH  * pixel_size / 2;
    bbox.max.y = pcenter.y + TFT_HEIGHT * pixel_size / 2;
}

Point16 toScreenCoords( Point16 p, Point16 screen_center)
{
    return Point16(
        ((p.x - screen_center.x) / pixel_size) + TFT_WIDTH / 2,
        ((p.y - screen_center.y) / pixel_size) + TFT_HEIGHT/ 2
    );
}


std::vector<Point16> clip_polygon( BBox bbox, std::vector<Point16> points)
{
    std::vector<Point16> clipped;
    int16_t dx, dy, bbpx;
    for( int i=0; i < (points.size() - 1); i++){
        Point16 p1 = points[i];
        Point16 p2 = points[i+1];
        // cut vert left side
        if( p1.x < bbox.min.x && p2.x > bbox.min.x) {
            dx = p2.x - p1.x;
            dy = abs( p2.y - p1.y);
            bbpx = bbox.min.x - p1.x;
            assert( dx > 0); assert( dy > 0); assert( bbpx > 0);
            p1.y = double(bbpx * dy) / dx;
            p1.x = bbox.min.x;
        }

        if( p1.x > bbox.min.x && p2.x < bbox.min.x) {

        }

    }
    return clipped;
}

Point32::Point32( char *coords_pair)
{
    char *next;
    x = round( strtod( coords_pair, &next));  // 1st coord
    y = round( strtod( ++next, NULL));  // 2nd coord
}

// void header_msg( String msg)
// {
//     tft.fillRect(0, 0, 240, 25, TFT_YELLOW);
//     tft.setCursor(5,5,2);
//     tft.print( msg);
// }



// Point16 clip_segment( Point16 p_in, Point16 p_out, BBox bbox)
// {
//     Point16 res;
//     if( p_out.x < bbox.min.x){
//         res.x = bbox.min.x;
//         // res.y =  
//     } else if( p_out.x > bbox.max.x){
//         res.x = bbox.max.x;
//         // res.y =  
//     } else{
//         res.x = p_out.x;
//     }

//     if( p_out.y < bbox.min.y){
//         res.y = bbox.min.y;
//         // res.x =
//     } else if( p_out.y > bbox.max.y){
//         res.y = bbox.max.y;
//         // res.x =  
//     } else {
//         res.y = p_out.y;
//     }
//     // WIP
// }