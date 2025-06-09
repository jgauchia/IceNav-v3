/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.2.3
 * @date 2025-06
 */

#include "mainScr.hpp"

bool isMainScreen = false;    // Flag to indicate main screen is selected
bool isScrolled = true;       // Flag to indicate when tileview was scrolled
bool isReady = false;         // Flag to indicate when tileview scroll was finished
bool isScrollingMap = false;  // Flag to indicate if map is scrolling
bool canScrollMap = false;    // Flag to indicate whet can scroll map
uint8_t activeTile = 0;       // Current active tile
uint8_t gpxAction = WPT_NONE; // Current Waypoint Action
int heading = 0;              // Heading value (Compass or GPS)
extern uint32_t DOUBLE_TOUCH_EVENT;

extern Compass compass;
extern Gps gps;
extern wayPoint loadWpt;

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
lv_obj_t *navArrow;
lv_obj_t *zoomWidget;
lv_obj_t *zoomLabel;
lv_obj_t *mapCanvas;
lv_layer_t canvasMapLayer;

extern Maps mapView;

/**
 * @brief Update compass screen event
 *
 * @param event
 */
void updateCompassScr(lv_event_t *event)
{
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
  if (obj == compassHeading)
  {
    lv_label_set_text_fmt(compassHeading, "%5d\xC2\xB0", heading);
    lv_img_set_angle(compassImg, -(heading * 10));
  }
  if (obj == latitude)
    lv_label_set_text_fmt(latitude, "%s", latFormatString(gps.gpsData.latitude));
  if (obj == longitude)
    lv_label_set_text_fmt(longitude, "%s", lonFormatString(gps.gpsData.longitude));
  if (obj == altitude)
    lv_label_set_text_fmt(obj, "%4d m.", gps.gpsData.altitude);
  if (obj == speedLabel)
    lv_label_set_text_fmt(obj, "%3d Km/h", gps.gpsData.speed);
  if (obj == sunriseLabel)
  {
    lv_label_set_text_static(obj, gps.gpsData.sunriseHour);
    lv_label_set_text_static(sunsetLabel, gps.gpsData.sunsetHour);
  }
}

/**
 * @brief Show Map Widgets
 */
void showMapWidgets()
{
  lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Hide Map Widgets
 *
 */
void hideMapWidgets()
{
  lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);  
  lv_obj_add_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
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
    mapView.redrawMap = true;

    if (activeTile == MAP)
    {
      mapView.createMapScrSprites();
      if (mapView.isMapFound)
        showMapWidgets();
      else
        hideMapWidgets();
    }

    if (isBarOpen)
      lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_HIDDEN);
  }
  else
    isReady = true;

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
  mapView.redrawMap = false;
  mapView.deleteMapScrSprites();
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
          heading = compass.getHeading();
        if (compass.isUpdated())
          lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
  #endif
  #ifndef ENABLE_COMPASS
        heading = gps.gpsData.heading;
        lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
  #endif
        if (gps.hasLocationChange())
        {
          lv_obj_send_event(latitude, LV_EVENT_VALUE_CHANGED, NULL);
          lv_obj_send_event(longitude, LV_EVENT_VALUE_CHANGED, NULL);
        }
        if (gps.isAltitudeChanged())
          lv_obj_send_event(altitude, LV_EVENT_VALUE_CHANGED, NULL);
        if (gps.isSpeedChanged())
          lv_obj_send_event(speedLabel, LV_EVENT_VALUE_CHANGED, NULL);
        break;

      case MAP:
  #ifdef ENABLE_COMPASS
        heading = compass.getHeading();
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
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

  if (showMapToolBar)
  {
    // if (activeTile == MAP && isMainScreen)
    // {
    //   switch (dir)
    //   {
    //     case LV_DIR_LEFT:
    //       // mapView.panMap(1,0);
    //       mapView.scrollMap(30,0);
    //       break;
    //     case LV_DIR_RIGHT:
    //       // mapView.panMap(-1,0);
    //       mapView.scrollMap(-30,0);
    //       break;
    //     case LV_DIR_TOP:
    //       //mapView.panMap(0,1);
    //       mapView.scrollMap(0,30);
    //       break;
    //     case LV_DIR_BOTTOM:
    //       // mapView.panMap(0,-1);
    //       mapView.scrollMap(0,-30);
    //       break;
    //   }
    // }
  }
}

/**
 * @brief Update map event
 *
 * @param event
 */
void updateMap(lv_event_t *event)
{
  if (mapSet.vectorMap)
    mapView.generateVectorMap(zoom);
  else
    mapView.generateRenderMap(zoom);

 if (mapView.redrawMap)
 {
    mapView.displayMap();
    lv_canvas_set_buffer(mapCanvas, mapView.mapBuffer, tft.width(), tft.height()-27, LV_COLOR_FORMAT_RGB565);
 }

}

/**
 * @brief Update Satellite Tracking
 *
 * @param event
 */
void updateSatTrack(lv_event_t *event)
{
  if (gps.isDOPChanged())
  {
    lv_label_set_text_fmt(pdopLabel, "PDOP: %.1f", gps.gpsData.pdop);
    lv_label_set_text_fmt(hdopLabel, "HDOP: %.1f", gps.gpsData.hdop);
    lv_label_set_text_fmt(vdopLabel, "VDOP: %.1f", gps.gpsData.vdop);
  }

  if (gps.isAltitudeChanged())
    lv_label_set_text_fmt(altLabel, "ALT: %4dm.", gps.gpsData.altitude);

  drawSatSNR();
  drawSatSky();
}

/**
 * @brief Map Tool Bar Event
 *
 * @param event
 */
void mapToolBarEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  showMapToolBar = !showMapToolBar;
  canScrollMap = !canScrollMap;

  if (!showMapToolBar)
  {
    lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
    mapView.centerOnGps(gps.gpsData.latitude, gps.gpsData.longitude);
    lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
    if (!mapView.followGps)
      lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
  }
}

/**
 * @brief Scrool Map Event
 *
 * @param event
 */
void scrollMapEvent(lv_event_t *event)
{
  if (canScrollMap)
  {
    lv_event_code_t code = lv_event_get_code(event);
    lv_indev_t * indev = lv_event_get_indev(event);
    static int last_x = 0, last_y = 0;
    static int dx = 0, dy = 0;
    lv_point_t p;


    switch (code)
    {
      case LV_EVENT_PRESSED:
      {
        lv_indev_get_point(indev, &p);
        last_x = p.x;
        last_y = p.y;
        isScrollingMap = true;
        break;
      }
  
      case LV_EVENT_PRESSING:
      {
        lv_indev_get_point(indev, &p);

        int dx = p.x - last_x;
        int dy = p.y - last_y;

        const int SCROLL_THRESHOLD = 5; 

        if (abs(dx) > SCROLL_THRESHOLD || abs(dy) > SCROLL_THRESHOLD) 
        {
          mapView.scrollMap(-dx, -dy);
          lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
          last_x = p.x;
          last_y = p.y;
        }
        break;
      }
  
      case LV_EVENT_PRESS_LOST:
      {
        lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
        isScrollingMap = false;
        break;
      }
    }
  }
}

/**
 * @brief Zoom Event Toolbar
 *
 * @param event
 */
void zoomEvent(lv_event_t *event)
{
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
  if ( obj == btnZoomIn )
  {
    if (!mapSet.vectorMap)
    {
      if (zoom >= minZoom && zoom < maxZoom)
        zoom++;
    }
    else
    {
      zoom--;
      mapView.isPosMoved = true;
      if (zoom < 1)
      {
        zoom = 1;
        mapView.isPosMoved = false;
      }
    }
  }
  else if ( obj == btnZoomOut )
  {
    if (!mapSet.vectorMap)
    {
      if (zoom <= maxZoom && zoom > minZoom)
        zoom--;
    }
    else
    {
      zoom++;
      mapView.isPosMoved = true;
      if (zoom > MAX_ZOOM)
      {
        zoom = MAX_ZOOM;
        mapView.isPosMoved = false;
      }
    }
  }
  lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
  lv_label_set_text_fmt(zoomLabel, "%2d", zoom);
}

/**
 * @brief Navigation update event
 *
 * @param event
 */
void updateNavEvent(lv_event_t *event)
{
  int wptDistance = (int)calcDist(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon);
  lv_label_set_text_fmt(distNav, "%d m.", wptDistance);

  if (wptDistance == 0)
  {
    LV_IMG_DECLARE(navfinish);
    lv_img_set_src(arrowNav, &navfinish);
    lv_img_set_angle(arrowNav, 0);
  }
  else
  {
#ifdef ENABLE_COMPASS
    double wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - compass.getHeading();
#endif
#ifndef ENABLE_COMPASS
    double wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - gps.gpsData.heading;
#endif
    lv_img_set_angle(arrowNav, (wptCourse * 10));
  }
}

 /**
 * @brief Create Canvas for Map
 *
 * @param screen 
 */
void createMapCanvas(_lv_obj_t *screen)
{
  // static lv_color_t *cbuf = (lv_color_t*)heap_caps_aligned_alloc(16, (tft.width()*tft.height()*sizeof(lv_color_t)), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);;
  mapCanvas = lv_canvas_create(screen);
  // lv_canvas_set_buffer(mapCanvas, cbuf, tft.width(), tft.height(), LV_COLOR_FORMAT_RGB565);
  // lv_canvas_fill_bg(mapCanvas, lv_color_white(), LV_OPA_100);
  // lv_canvas_init_layer(constCanvas, &canvasMapLayer);
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
  lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
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

  // Map Tile
  createMapCanvas(mapTile);

  // Navigation Arrow Widget
  navArrow = lv_img_create(mapTile);
  lv_img_set_src(navArrow, navArrowIconFile);
  lv_obj_align(navArrow, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);

  // Map zoom Widget
  zoomWidget = lv_obj_create(mapTile);
  lv_obj_set_size(zoomWidget, 64, 32);
  lv_obj_clear_flag(zoomWidget, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(zoomWidget, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(zoomWidget, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_color(zoomWidget, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(zoomWidget, 128, 0);
  lv_obj_set_style_border_color(zoomWidget, lv_color_white(), 0);
  lv_obj_set_style_border_width(zoomWidget, 1, 0);
  lv_obj_set_style_border_opa(zoomWidget,128,0);
  lv_obj_t *img = lv_img_create(zoomWidget);
  lv_img_set_src(img, zoomIconFile);
  zoomLabel = lv_label_create(zoomWidget);
  lv_obj_set_style_text_font(zoomLabel, &lv_font_montserrat_20, 0);
  lv_label_set_text_fmt(zoomLabel, "%2d", zoom);
  
  // Map Tile Toolbar
  btnZoomOut = lv_img_create(mapTile);
  lv_img_set_src(btnZoomOut, zoomOutIconFile);
  lv_img_set_zoom(btnZoomOut,buttonScale);
  lv_obj_update_layout(btnZoomOut);
  lv_obj_set_size(btnZoomOut,  48 * scaleBut, 48 * scaleBut);
  lv_obj_add_event_cb(btnZoomOut, zoomEvent, LV_EVENT_CLICKED, NULL);

  btnZoomIn = lv_img_create(mapTile);
  lv_img_set_src(btnZoomIn, zoomInIconFile);
  lv_img_set_zoom(btnZoomIn,buttonScale);
  lv_obj_update_layout(btnZoomIn);
  lv_obj_set_size(btnZoomIn,  48 * scaleBut, 48 * scaleBut);
  lv_obj_add_event_cb(btnZoomIn, zoomEvent, LV_EVENT_CLICKED, NULL);

  lv_obj_set_pos(btnZoomOut, 10, mapView.mapScrHeight - toolBarOffset);
  lv_obj_set_pos(btnZoomIn, 10, mapView.mapScrHeight - (toolBarOffset + toolBarSpace));

  if (!showMapToolBar)
  {
    lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
  }

  // Map Tile Events
  lv_obj_add_event_cb(mapTile, updateMap, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(mainScreen, gestureEvent, LV_EVENT_GESTURE, NULL);
  DOUBLE_TOUCH_EVENT = lv_event_register_id();
  lv_obj_add_event_cb(mapTile, mapToolBarEvent, (lv_event_code_t)DOUBLE_TOUCH_EVENT, NULL);
  lv_obj_add_event_cb(mapTile, scrollMapEvent, LV_EVENT_ALL, NULL);

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
  lv_obj_set_pos(constCanvas, (TFT_WIDTH / 2) - canvasCenter_X, 240);
#endif
#ifdef TDECK_ESP32S3
  createConstCanvas(constMsg);
  lv_obj_align(constCanvas, LV_ALIGN_CENTER, 0, 0);
  drawSatConst();
#endif
#endif

  // Satellite Tracking Event
  lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);
}
