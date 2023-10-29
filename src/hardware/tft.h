/**
 * @file tft.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.1.6
 * @date 2023-06-14
 */

// #include <SPIFFS.h>
#define CALIBRATION_FILE "/TouchCalData1"
bool REPEAT_CAL = false;

// #ifdef CUSTOMBOARD
// #include <LGFX_CUSTOMBOARD.hpp>
// #endif

// #ifdef MAKERF_ESP32S3
// #include <LGFX_MakerFabs_Parallel_S3.hpp>
// #endif

// #include <LGFX_TFT_eSPI.hpp>

uint8_t brightness_level = 255;

// static TFT_eSPI tft;
#define LVGL_BKG 0x10A3

/**
 * @brief Set the TFT brightness
 *
 * @param brightness -> 0..255
 */
void set_brightness(uint8_t brightness)
{
  if (brightness <= 255)
  {
    ledcWrite(0, brightness);
    brightness_level = brightness;
  }
}

/**
 * @brief Get the TFT brightness
 *
 * @return int -> brightness value 0..255
 */
uint8_t get_brightness()
{
  return brightness_level;
}

/**
 * @brief Turn on TFT Sleep Mode for ILI9488
 *
 */
void tft_on()
{
  tft.writecommand(0x11);
  set_brightness(255);
}

/**
 * @brief Turn off TFT Wake up Mode for ILI9488
 *
 */
void tft_off()
{
  tft.writecommand(0x10);
  set_brightness(0);
}

/**
 * @brief Touch calibrate
 *
 */
void touch_calibrate()
{
  uint16_t calData[8];
  uint8_t calDataOK = 0;

  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    if (REPEAT_CAL)
      SPIFFS.remove(CALIBRATION_FILE);
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 16) == 16)
          calDataOK = 1;
        f.close();
      }
      else
        log_v("Error opening touch configuration");
    }
  }

  if (calDataOK && !REPEAT_CAL)
    tft.setTouchCalibrate(calData);
  else
  {
    tft.drawCenterString("TOUCH THE ARROW MARKER.", 160, tft.height() >> 1, &fonts::DejaVu18);
    tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
    tft.drawCenterString("DONE!", 160, (tft.height() >> 1) + 30, &fonts::DejaVu40);
    delay(500);
    tft.drawCenterString("TOUCH TO CONTINUE.", 160, (tft.height() >> 1) + 100, &fonts::DejaVu18);

    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calData, 16);
      f.close();
    }

    uint16_t touchX, touchY;
    while (!tft.getTouch(&touchX, &touchY))
    {
    };
  }
}

/**
 * @brief Init tft display
 *
 */
void init_tft()
{
  tft.init();
  tft.setRotation(8);
  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();

  gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);
  ledcAttachPin(TFT_BL, 0);
  ledcSetup(0, 5000, 8);
  ledcWrite(0, 255);
#ifndef MAKERF_ESP32S3
  touch_calibrate();
#endif
}

/***********************************************************************************/

void header_msg( String msg)
{
    tft.fillRect(0, 0, 240, 25, TFT_YELLOW);
    tft.setCursor(5,5,2);
    tft.print( msg);
}


void fill_polygon( std::vector<Point16> points, int color) // scanline fill algorithm
{
    int16_t maxy = INT16_MIN, miny = INT16_MAX;

    for( Point16 p : points) { // TODO: precalculate at map file creation
        maxy = max( maxy, p.y);
        miny = min( miny, p.y);
    }
    if( maxy > TFT_HEIGHT) maxy = TFT_HEIGHT;
    if( miny < 0) miny = 0;
    assert( miny < maxy);

    // log_d("miny: %i, maxy: %i", miny, maxy);
    int16_t nodeX[points.size()], pixelY;

    //  Loop through the rows of the image.
    for( pixelY=miny; pixelY < maxy; pixelY++) {
        //  Build a list of nodes.
        int16_t nodes=0;
        for( int i=0; i < (points.size() - 1); i++) {
            if( (points[i].y < pixelY && points[i+1].y >= pixelY) ||
                (points[i].y >= pixelY && points[i+1].y < pixelY)) {
                    nodeX[nodes++] = 
                        points[i].x + double(pixelY-points[i].y)/double(points[i+1].y-points[i].y) * 
                        double(points[i+1].x-points[i].x);
                }
        }
        assert( nodes < points.size());
        // log_d("pixelY: %i, nodes: %i", pixelY, nodes);

        //  Sort the nodes, via a simple “Bubble” sort.
        int16_t i=0, swap;
        while( i < nodes-1) {   // TODO: rework
            if( nodeX[i] > nodeX[i+1]) {
                swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; 
                i=0;  
            }
            else { i++; }
        }

        //  Fill the pixels between node pairs.
        // log_d("Polygon: %i, %i", nodes, color);
        for (i=0; i <= nodes-2; i+=2) {
            if( nodeX[i] >= TFT_WIDTH) break;
            if( nodeX[i+1] <= 0 ) continue;
            if (nodeX[i] < 0 ) nodeX[i] = 0;
            if (nodeX[i+1] > TFT_WIDTH) nodeX[i+1]=TFT_WIDTH;
            // log_d("drawLine: %i, %i, %i, %i", nodeX[i], pixelY, nodeX[i+1], pixelY);
            tft.drawLine( nodeX[i], TFT_HEIGHT - pixelY, nodeX[i+1], TFT_HEIGHT - pixelY, color);
        }
    }
}


void draw( ViewPort& viewPort, MemBlocks& memblocks)
{
    // tft.fillScreen( BACKGROUND_COLOR);
    std::vector<Polygon> polygons_to_draw;
    std::vector<Polyline> lines_to_draw;
    for( MapBlock* mblock: memblocks.blocks){
        if( !mblock || !mblock->inView) continue;
        Point16 screen_center_mc = viewPort.center - mblock->offset;  // screen center with features coordinates
        BBox screen_bbox_mc = viewPort.bbox - mblock->offset;  // screen boundaries with features coordinates
        
        ////// Polygons 
        for( Polygon polygon : mblock->polygons){
            if( polygon.color == YELLOW) log_w("Polygon type unknown");
            Polygon new_polygon;
            bool hit = false;
            for( Point16 p : polygon.points){
                if( screen_bbox_mc.contains_point( p)) hit = true;
                new_polygon.points.push_back( toScreenCoords( p, screen_center_mc));
            }
            if( hit){
                new_polygon.color = polygon.color;
                polygons_to_draw.push_back( new_polygon);
            }
        }
        
        ////// Lines 
        for( Polyline line : mblock->polylines){
            Polyline new_line;
            new_line.color = line.color;
            new_line.width = line.width;
            bool prev_in_screen = false;
            for( int i=0; i < (line.points.size()); i++) {
                bool curr_in_screen = screen_bbox_mc.contains_point( line.points[i]);
                if( !prev_in_screen && !curr_in_screen){  // TODO: clip, the segment could still intersect the screen area!
                    prev_in_screen = false;
                    continue;
                    }
                if( prev_in_screen && !curr_in_screen){  // split polyline: end and start new polyline. Driver does the clipping of the segment.
                    new_line.points.push_back( toScreenCoords( line.points[i], screen_center_mc));  
                    lines_to_draw.push_back( new_line);
                    new_line.points.clear();
                    prev_in_screen = false;
                    continue;
                }
                if( !prev_in_screen && curr_in_screen && i > 0){  // reenter screen.  Driver does the clipping
                    new_line.points.push_back( toScreenCoords( line.points[i-1], screen_center_mc));
                }
                new_line.points.push_back( toScreenCoords( line.points[i], screen_center_mc));
                prev_in_screen = curr_in_screen;
            }
            assert( new_line.points.size() != 1);
            if( new_line.points.size() >= 2){
                lines_to_draw.push_back( new_line);
            }
        }
        tft.fillScreen( BACKGROUND_COLOR);
    }

    for( Polygon pol: polygons_to_draw){
        fill_polygon( pol.points, pol.color);
    }
    for( Polyline line: lines_to_draw){
        for( int i=0; i < (line.points.size() - 1); i++) {
            if( line.points[i].x < 0 || line.points[i+1].x < 0 ||
                line.points[i].x > TFT_WIDTH || line.points[i+1].x > TFT_WIDTH ||
                line.points[i].y < 0 || line.points[i].y > TFT_HEIGHT ||
                line.points[i+1].y < 0 || line.points[i+1].y > TFT_HEIGHT ){
                    log_d("Error: point out of screen: %i, %i, %i, %i", line.points[i].x, line.points[i].y, line.points[i+1].x, line.points[i+1].y);
                    // continue;
                }
            //tft.drawWideLine(
                tft.drawLine(
                line.points[i].x, TFT_HEIGHT - line.points[i].y,
                line.points[i+1].x, TFT_HEIGHT - line.points[i+1].y,
                //line.width/pixel_size ?: 1, line.color, line.color);
                 line.color);
        }      
    }

    tft.fillTriangle( 
        TFT_WIDTH/2 - 4, TFT_HEIGHT/2 + 5, 
        TFT_WIDTH/2 + 4, TFT_HEIGHT/2 + 5, 
        TFT_WIDTH/2,     TFT_HEIGHT/2 - 6, 
        RED);
    log_v("Draw done!");
}