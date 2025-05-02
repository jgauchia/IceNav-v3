/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.2.0
 * @date 2024-12
 */

#include "mainScr.hpp"

bool isMainScreen = false;    // Flag to indicate main screen is selected
bool isScrolled = true;       // Flag to indicate when tileview was scrolled
bool isReady = false;         // Flag to indicate when tileview scroll was finished
bool redrawMap = true;        // Flag to indicate when needs to redraw Map
uint8_t activeTile = 0;       // Current active tile
uint8_t wptAction = WPT_NONE; // Current Waypoint Action
int wptPosX, wptPosY = 0;     // Waypoint position on map

#ifdef LARGE_SCREEN
  uint8_t toolBarOffset = 100;
  uint8_t toolBarSpace = 60;
#endif
#ifndef LARGE_SCREEN
  uint8_t toolBarOffset = 80;
  uint8_t toolBarSpace = 50;
#endif

lv_obj_t *tilesScreen;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *btnFullScreen;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;

double destLat = 0;
double destLon = 0;
char* destName = (char *)"";


/**
 * @brief Update compass screen event
 *
 * @param event
 */
void updateCompassScr(lv_event_t * event)
{
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
  if (obj==compassHeading)
  {
    lv_label_set_text_fmt(compassHeading, "%5d\xC2\xB0", heading);
    lv_img_set_angle(compassImg, -(heading * 10));
  }
  if (obj==latitude)
    lv_label_set_text_fmt(latitude, "%s", latFormatString(gpsData.latitude));
  if (obj==longitude)
    lv_label_set_text_fmt(longitude, "%s", lonFormatString(gpsData.longitude));
  if (obj==altitude)
    lv_label_set_text_fmt(obj, "%4d m.", gpsData.altitude);
  if (obj==speedLabel)
    lv_label_set_text_fmt(obj, "%3d Km/h", gpsData.speed);
  if (obj==sunriseLabel)
    lv_label_set_text_static(obj, gpsData.sunriseHour);
  if (obj==sunsetLabel)
    lv_label_set_text_static(obj, gpsData.sunsetHour);
}

/**
 * @brief Get the active tile
 *
 * @param event
 */
void getActTile(lv_event_t *event)
{
  if (isReady)
  {
    isScrolled = true;
    redrawMap = true;

    if (activeTile == MAP)
    {
      createMapScrSprites();
      if (isMapFullScreen)
      {
        lv_obj_add_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(menuBtn,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(notifyBarHour, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(notifyBarIcons, LV_OBJ_FLAG_HIDDEN);
      }
      else
      {
        lv_obj_clear_flag(notifyBarHour,LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(notifyBarIcons, LV_OBJ_FLAG_HIDDEN);     
        lv_obj_clear_flag(menuBtn,LV_OBJ_FLAG_HIDDEN);

        if (isBarOpen)
          lv_obj_clear_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);
        else 
          lv_obj_add_flag(buttonBar, LV_OBJ_FLAG_HIDDEN);
      }
    }
    else if (activeTile != MAP)
    {
      lv_obj_clear_flag(menuBtn,LV_OBJ_FLAG_HIDDEN);

      if (isBarOpen)
         lv_obj_clear_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);

    }
  }
  else
  {
    isReady = true;
  }

  lv_obj_t *actTile = lv_tileview_get_tile_act(tilesScreen);
  lv_coord_t tileX = lv_obj_get_x(actTile) / TFT_WIDTH;
  activeTile = tileX;
}

/**
 * @brief Tile start scrolling event
 *
 * @param event
 */
void scrollTile(lv_event_t *event)
{
  isScrolled = false;
  isReady = false;
  redrawMap = false;

  if (isMapFullScreen)
  {
    lv_obj_clear_flag(notifyBarHour,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(notifyBarIcons, LV_OBJ_FLAG_HIDDEN);
  }

  deleteMapScrSprites();
}

/**
 * @brief Update Main Screen
 *
 */
void updateMainScreen(lv_timer_t *t)
{
  if (isScrolled && isMainScreen)
  {
    switch (activeTile)
    {
      case COMPASS:
        #ifdef ENABLE_COMPASS
        if (!waitScreenRefresh)
          heading = getHeading();
        #endif
        #ifndef ENABLE_COMPASS
          heading = gpsData.heading;
        #endif
        lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(latitude, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(longitude, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(altitude, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(speedLabel, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(sunriseLabel, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_send_event(sunsetLabel, LV_EVENT_VALUE_CHANGED, NULL);
        break;
      
      case MAP:
        #ifdef ENABLE_COMPASS
          heading = getHeading();
        #endif
        lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
        break;
          
      case NAV:
        lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
        break;

      case SATTRACK:
        lv_obj_send_event(satTrackTile, LV_EVENT_VALUE_CHANGED, NULL);
        break;
          

      default:
        break;
    }
  }
}

/**
 * @brief Map Gesture Event
 *
 * @param event
 */
void gestureEvent(lv_event_t *event)
{
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (activeTile == MAP && isMainScreen)
  {
    switch (dir)
    {
      case LV_DIR_LEFT:
        break;
      case LV_DIR_RIGHT:
        break;
      case LV_DIR_TOP:
        break;
      case LV_DIR_BOTTOM:
        break;
    }
  }
}

/**
 * @brief Update map event
 *
 * @param event
 */
void updateMap(lv_event_t *event)
{
  if (isVectorMap)
  {
    getPosition(gpsData.latitude, gpsData.longitude);
    if (isPosMoved)
    {
      tileSize = VECTOR_TILE_SIZE;
      viewPort.setCenter(point);

      getMapBlocks(viewPort.bbox, memCache);
             
      generateVectorMap(viewPort, memCache, mapTempSprite); 
      
      isPosMoved = false;
    }
  }
  else
  {
    tileSize = RENDER_TILE_SIZE;
    generateRenderMap();
  }
  if (redrawMap)
    displayMap(tileSize);
}

/**
 * @brief Update Satellite Tracking
 *
 * @param event
 */
void updateSatTrack(lv_event_t *event)
{
  lv_label_set_text_fmt(pdopLabel, "PDOP: %.1f", gpsData.pdop);
  lv_label_set_text_fmt(hdopLabel, "HDOP: %.1f", gpsData.hdop);
  lv_label_set_text_fmt(vdopLabel, "VDOP: %.1f", gpsData.vdop);

  lv_label_set_text_fmt(altLabel, "ALT: %4dm.", gpsData.altitude);

  drawSatSNR();
  drawSatSky();
}

/**
 * @brief Tool Bar Event
 *
 * @param event
 */
void toolBarEvent(lv_event_t *event)
{
  showToolBar = !showToolBar;

  if (!isMapFullScreen)
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT - toolBarOffset);
    lv_obj_set_pos(btnZoomOut, 10 , MAP_HEIGHT - (toolBarOffset + toolBarSpace));
    lv_obj_set_pos(btnZoomIn, 10, MAP_HEIGHT - (toolBarOffset + (2 * toolBarSpace)));
  }
  else
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT_FULL - (toolBarOffset + 24));
    lv_obj_set_pos(btnZoomOut, 10, MAP_HEIGHT_FULL - (toolBarOffset + toolBarSpace + 24));
    lv_obj_set_pos(btnZoomIn,10, MAP_HEIGHT_FULL - (toolBarOffset + (2 * toolBarSpace) + 24));
  }

  if (!showToolBar)
  {
    lv_obj_clear_flag(btnFullScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_CLICKABLE);
  }
  else
  {
    lv_obj_add_flag(btnFullScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_CLICKABLE);
  }
}

/**
 * @brief Full Screen Event Toolbar
 *
 * @param event
 */
void fullScreenEvent(lv_event_t *event)
{
  isMapFullScreen = !isMapFullScreen;

  if (!isMapFullScreen)
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT - toolBarOffset);
    lv_obj_set_pos(btnZoomOut, 10, MAP_HEIGHT - (toolBarOffset + toolBarSpace));
    lv_obj_set_pos(btnZoomIn, 10, MAP_HEIGHT - (toolBarOffset + (2 * toolBarSpace)));

    if (isBarOpen)
      lv_obj_clear_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(menuBtn,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(notifyBarHour, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(notifyBarIcons, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT_FULL - (toolBarOffset + 24));
    lv_obj_set_pos(btnZoomOut, 10, MAP_HEIGHT_FULL - (toolBarOffset + toolBarSpace + 24));
    lv_obj_set_pos(btnZoomIn, 10, MAP_HEIGHT_FULL - (toolBarOffset + (2 * toolBarSpace) + 24));
    lv_obj_add_flag(buttonBar,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(menuBtn,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notifyBarHour, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notifyBarIcons, LV_OBJ_FLAG_HIDDEN);
  }
  
  deleteMapScrSprites();
  createMapScrSprites();

  redrawMap = true;

  lv_obj_invalidate(tilesScreen);   
  lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
}

/**
 * @brief Zoom In Event Toolbar
 *
 * @param event
 */
void zoomInEvent(lv_event_t *event)
{
  if (!isVectorMap)
  {
    if (zoom >= minZoom && zoom < maxZoom)
      zoom++;
  }
  else
  {
    zoom--;
    isPosMoved = true;
    if (zoom < 1)
    {
      zoom = 1;
      isPosMoved = false;
    }
  }

  lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
}

/**
 * @brief Zoom Out Event Toolbar
 *
 * @param event
 */
void zoomOutEvent(lv_event_t *event)
{
  if (!isVectorMap)
  {
    if (zoom <= maxZoom && zoom > minZoom)
      zoom--;
  }
  else
  {
    zoom++;
    isPosMoved = true;
    if (zoom > MAX_ZOOM)
    {
      zoom = MAX_ZOOM;
      isPosMoved = false;
    }
  }

  lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
}

/**
 * @brief Navigation update event
 *
 * @param event
 */
void updateNavEvent(lv_event_t *event)
{
  int wptDistance = (int)calcDist(gpsData.latitude, gpsData.longitude, destLat, destLon);
  lv_label_set_text_fmt(distNav,"%d m.", wptDistance);

  if (wptDistance == 0)
  {
    lv_img_set_src(arrowNav, &navfinish);
    lv_img_set_angle(arrowNav, 0);
  }
  else
  {
    #ifdef ENABLE_COMPASS
      double wptCourse = calcCourse(gpsData.latitude, gpsData.longitude, loadWpt.lat, loadWpt.lon) - getHeading();
    #endif
    #ifndef ENABLE_COMPASS
      double wptCourse = calcCourse(gpsData.latitude, gpsData.longitude, loadWpt.lat, loadWpt.lon) - gpsData.heading;
    #endif
    lv_img_set_angle(arrowNav, (wptCourse * 10));
  }
}

/**
 * @brief Create Main Screen
 *
 */
void createMainScr()
{
  mainScreen = lv_obj_create(NULL);
  
  // Main Screen Tiles
  tilesScreen = lv_tileview_create(mainScreen);
  compassTile = lv_tileview_add_tile(tilesScreen, 0, 0, LV_DIR_RIGHT);
  mapTile = lv_tileview_add_tile(tilesScreen, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
  navTile = lv_tileview_add_tile(tilesScreen, 2, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
  lv_obj_add_flag(navTile,LV_OBJ_FLAG_HIDDEN); 
  satTrackTile = lv_tileview_add_tile(tilesScreen, 3, 0, LV_DIR_LEFT);
  lv_obj_set_size(tilesScreen, TFT_WIDTH, TFT_HEIGHT - 25);
  lv_obj_set_pos(tilesScreen, 0, 25);
  static lv_style_t styleScroll;
  lv_style_init(&styleScroll);
  lv_style_set_bg_color(&styleScroll, lv_color_hex(0xFFFFFF));
  lv_obj_add_style(tilesScreen, &styleScroll, LV_PART_SCROLLBAR);
  // Main Screen Events
  lv_obj_add_event_cb(tilesScreen, getActTile, LV_EVENT_SCROLL_END, NULL);
  lv_obj_add_event_cb(tilesScreen, scrollTile, LV_EVENT_SCROLL_BEGIN, NULL);
  
  // Compass Tile
   
  // Compass Widget
  compassWidget(compassTile);
  // Position widget
  positionWidget(compassTile);
  // Altitude widget
  altitudeWidget(compassTile);
  // Speed widget
  speedWidget(compassTile);
  // Sunrise/Sunset widget
  sunWidget(compassTile);
  
  // Compass Tile Events
  lv_obj_add_event_cb(compassHeading, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(latitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(longitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(altitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(speedLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sunriseLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sunsetLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
 
  // Map Tile Toolbar
  btnFullScreen = lv_btn_create(mapTile);
  lv_obj_remove_style_all(btnFullScreen);
  lv_obj_set_size(btnFullScreen, 48, 48); 
  lv_obj_add_event_cb(btnFullScreen, fullScreenEvent, LV_EVENT_CLICKED, NULL);

  btnZoomOut = lv_btn_create(mapTile);
  lv_obj_remove_style_all(btnZoomOut);
  lv_obj_set_size(btnZoomOut, 48, 48);
  lv_obj_add_event_cb(btnZoomOut, zoomOutEvent, LV_EVENT_CLICKED, NULL);


  btnZoomIn = lv_btn_create(mapTile);
  lv_obj_remove_style_all(btnZoomIn);
  lv_obj_set_size(btnZoomIn, 48, 48);
  lv_obj_add_event_cb(btnZoomIn, zoomInEvent, LV_EVENT_CLICKED, NULL);

  if (!isMapFullScreen)
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT - toolBarOffset);
    lv_obj_set_pos(btnZoomOut, 10, MAP_HEIGHT - (toolBarOffset + toolBarSpace));
    lv_obj_set_pos(btnZoomIn, 10, MAP_HEIGHT - ( toolBarOffset + (2 * toolBarSpace)));
  }
  else
  {
    lv_obj_set_pos(btnFullScreen, 10, MAP_HEIGHT_FULL - (toolBarOffset + 24));
    lv_obj_set_pos(btnZoomOut, 10, MAP_HEIGHT_FULL - (toolBarOffset + toolBarSpace + 24));
    lv_obj_set_pos(btnZoomIn, 10, MAP_HEIGHT_FULL - (toolBarOffset + (2 * toolBarSpace) + 24));
  }

  if (!showToolBar)
  {
    lv_obj_clear_flag(btnFullScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_CLICKABLE);
  }
  else
  {
    lv_obj_add_flag(btnFullScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_CLICKABLE);
  }

  // Map Tile Events
  lv_obj_add_event_cb(mapTile, updateMap, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(mapTile, gestureEvent, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(mapTile, toolBarEvent, LV_EVENT_LONG_PRESSED, NULL);
  
  // Navigation Tile
  navigationScr(navTile);

  // Navigation Tile Events
  lv_obj_add_event_cb(navTile, updateNavEvent, LV_EVENT_VALUE_CHANGED, NULL);
  
  // Satellite Tracking and info Tile
  satelliteScr(satTrackTile);
  #ifdef BOARD_HAS_PSRAM
    #ifndef TDECK_ESP32S3
      createConstCanvas(satTrackTile);
      drawSatConst();
      lv_obj_set_pos(constCanvas,( TFT_WIDTH / 2 ) - canvasCenter_X, 240);
    #endif
    #ifdef TDECK_ESP32S3
      createConstCanvas(constMsg);
      lv_obj_align(constCanvas,LV_ALIGN_CENTER,0,0);
      drawSatConst();
    #endif
  #endif

  // Satellite Tracking Event
  lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);
}

