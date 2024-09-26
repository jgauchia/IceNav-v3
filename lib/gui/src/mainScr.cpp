/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Main Screen
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "mainScr.hpp"
#include "buttonBar.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "globalGuiDef.h"
#include "globalMapsDef.h"
#include "settings.hpp"
#include "tft.hpp"

extern const int SD_CS;
extern const uint8_t TFT_SPI_CS;

bool isMainScreen = false;    // Flag to indicate main screen is selected
bool isScrolled = true;       // Flag to indicate when tileview was scrolled
bool isReady = false;         // Flag to indicate when tileview scroll was finished
bool redrawMap = true;        // Flag to indicate when needs to redraw Map
uint8_t activeTile = 0;       // Current active tile
uint8_t wptAction = WPT_NONE; // Current Waypoint Action
int wptPosX, wptPosY = 0;     // Waypoint position on map

#ifdef LARGE_SCREEN
  int toolBarOffset = 100;
  int toolBarSpace = 60;
#endif
#ifndef LARGE_SCREEN
  int toolBarOffset = 80;
  int toolBarSpace = 50;
#endif

lv_obj_t *tilesScreen;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *btnFullScreen;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;
lv_obj_t *nameNav;
lv_obj_t *latNav;
lv_obj_t *lonNav;
lv_obj_t *distNav;
lv_obj_t *arrowNav;

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
    #ifdef ENABLE_COMPASS
      lv_label_set_text_fmt(compassHeading, "%5d\xC2\xB0", heading);
      lv_img_set_angle(compassImg, -(heading * 10));
    #endif
  }
  if (obj==latitude)
    lv_label_set_text_fmt(latitude, "%s", latFormatString(getLat()));
  if (obj==longitude)
    lv_label_set_text_fmt(longitude, "%s", lonFormatString(getLon()));
  if (obj==altitude)
    lv_label_set_text_fmt(obj, "%4d m.", (int)GPS.altitude.meters());
  if (obj==speedLabel)
    lv_label_set_text_fmt(obj, "%3d Km/h", (int)GPS.speed.kmph());
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

    if (activeTile == SATTRACK)
    {
      createSatSprite(spriteSat);
      createConstelSprite(constelSprite);
    }
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
  deleteSatInfoSprites();
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
        lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
        
        
        if(GPS.location.isValid())
        {
          lv_obj_send_event(latitude, LV_EVENT_VALUE_CHANGED, NULL);
          lv_obj_send_event(longitude, LV_EVENT_VALUE_CHANGED, NULL);
        }
        if (GPS.altitude.isValid())
        {
          lv_obj_send_event(altitude, LV_EVENT_VALUE_CHANGED, NULL);
        }

        if (GPS.speed.isValid())
          lv_obj_send_event(speedLabel, LV_EVENT_VALUE_CHANGED, NULL);
        break;
      
      case MAP:
        // if (GPS.location.isUpdated())
        #ifdef ENABLE_COMPASS
          heading = getHeading();
        #endif
        lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
        break;
          
      case NAV:
        lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
        break;

      case SATTRACK:
        constelSprite.pushSprite(150 * scale, 40 * scale);
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
  lv_obj_t *screen = (lv_obj_t *)lv_event_get_current_target(event);
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
    getPosition(getLat(), getLon());
    if (isPosMoved)
    {
      tileSize = VECTOR_TILE_SIZE;
      viewPort.setCenter(point);

      acquireSdSPI();
      
      getMapBlocks(viewPort.bbox, memCache);
      
      releaseSdSPI();
            
      // deleteMapScrSprites();
      // createMapScrSprites();
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
 * @brief GNSS Selection Checkbox event
 *
 * @param event
 */
void activeGnssEvent(lv_event_t *event)
{
  uint32_t *activeId = (uint32_t *)lv_event_get_user_data(event);
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_current_target(event);
  lv_obj_t *activeCheckBox = (lv_obj_t *)lv_event_get_target(event);
  lv_obj_t *oldCheckBox = lv_obj_get_child(cont, *activeId);
  
  if (activeCheckBox == cont)
    return;
  
  lv_obj_clear_state(oldCheckBox, LV_STATE_CHECKED);
  lv_obj_add_state(activeCheckBox, LV_STATE_CHECKED);
  
  clearSatInView();
  
  *activeId = lv_obj_get_index(activeCheckBox);
}

/**
 * @brief Update Satellite Tracking
 *
 * @param event
 */
void updateSatTrack(lv_event_t *event)
{
  if (pdop.isUpdated() || hdop.isUpdated() || vdop.isUpdated())
  {
    lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", pdop.value());
    lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", hdop.value());
    lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", vdop.value());
  }

  if (GPS.altitude.isUpdated())
    lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", (int)GPS.altitude.meters());

  #ifdef AT6558D_GPS
    switch ((int)activeGnss)
    {
      case 0:
        fillSatInView(GPS_GSV, TFT_GREEN);
        break;
      case 1:
        fillSatInView(GL_GSV, TFT_BLUE);
        break;
      case 2:
        fillSatInView(BD_GSV, TFT_RED);
        break;
    }
  #else
    fillSatInView(GPS_GSV, TFT_GREEN);
  #endif
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
  int wptDistance = (int)calcDist(getLat(), getLon(), destLat, destLon);
  lv_label_set_text_fmt(distNav,"%d m.", wptDistance);

  if (wptDistance == 0)
  {
    lv_img_set_src(arrowNav, &navfinish);
    #ifdef ENABLE_COMPASS
      lv_img_set_angle(arrowNav, 0);
    #endif
  }
  else
  {
    #ifdef ENABLE_COMPASS
      double wptCourse = calcCourse(getLat(), getLon(), loadWpt.lat, loadWpt.lon) - getHeading();
      lv_img_set_angle(arrowNav, (wptCourse * 10));
    #endif
    #ifndef ENABLE_COMPASS
       lv_img_set_src(arrowNav, NULL);
    #endif
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
  
  // Compass Tile Events
  lv_obj_add_event_cb(compassHeading, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(latitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(longitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(altitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(speedLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
 
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
  lv_obj_t * label;
  label = lv_label_create(navTile);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Navigation to:");
  lv_obj_center(label);
  lv_obj_align(label,LV_ALIGN_TOP_LEFT,10, 20);

  nameNav = lv_label_create(navTile);
  lv_obj_set_style_text_font(nameNav, fontLargeMedium, 0);
  //lv_label_set_text_fmt(nameNav, "%s","");
  lv_obj_set_width(nameNav,TFT_WIDTH-10);
  lv_obj_set_pos(nameNav,10, 55);

  label = lv_label_create(navTile);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lat:");
  lv_obj_set_pos(label, 10, 90);

  label = lv_label_create(navTile);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lon:");
  lv_obj_set_pos(label, 10, 120);

  latNav = lv_label_create(navTile);
  lv_obj_set_style_text_font(latNav, fontOptions, 0);
  lv_label_set_text_fmt(latNav, "%s", "");
  lv_obj_set_pos(latNav, 60, 90);
  
  lonNav = lv_label_create(navTile);
  lv_obj_set_style_text_font(lonNav, fontOptions, 0);
  lv_label_set_text_fmt(lonNav, "%s", "");
  lv_obj_set_pos(lonNav, 60, 120);

  label = lv_label_create(navTile);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Distance");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

  distNav = lv_label_create(navTile);
  lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
  lv_label_set_text_fmt(distNav,"%d m.", 0);
  lv_obj_align(distNav,LV_ALIGN_CENTER, 0, -5);

  arrowNav = lv_img_create(navTile);
  lv_img_set_zoom(arrowNav,iconScale);
  lv_obj_update_layout(arrowNav);
  lv_obj_align(arrowNav,LV_ALIGN_CENTER, 0, 100);
  
  #ifdef ENABLE_COMPASS
    lv_img_set_src(arrowNav, &navup);
    lv_img_set_pivot(arrowNav, 50, 50) ;
  #endif
  
  // Navigation Tile Events
  lv_obj_add_event_cb(navTile, updateNavEvent, LV_EVENT_VALUE_CHANGED, NULL);
  
  // Satellite Tracking Tile
  lv_obj_t *infoGrid = lv_obj_create(satTrackTile);
  lv_obj_set_size(infoGrid, 90, 175);
  lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(infoGrid, 5 * scale, 0);
  lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_COLUMN);
  static lv_style_t styleGrid;
  lv_style_init(&styleGrid);
  lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
  lv_style_set_border_opa(&styleGrid, LV_OPA_0);
  lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
  lv_obj_set_y(infoGrid,0);
  
  pdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(pdopLabel, fontSatInfo, 0);
  lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", "0.0");
  
  hdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(hdopLabel, fontSatInfo, 0);
  lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", "0.0");
  
  vdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(vdopLabel, fontSatInfo, 0);
  lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", "0.0");
  
  altLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(altLabel, fontSatInfo, 0);
  lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", 0); 
  
  satelliteBar1 = lv_chart_create(satTrackTile);
  lv_obj_set_size(satelliteBar1, TFT_WIDTH, 55 * scale);
  lv_chart_set_div_line_count(satelliteBar1, 6, 0);
  lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
  satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
  lv_chart_set_point_count(satelliteBar1, (MAX_SATELLLITES_IN_VIEW / 2));
  lv_obj_set_pos(satelliteBar1, 0, 175 * scale);
  
  satelliteBar2 = lv_chart_create(satTrackTile);
  lv_obj_set_size(satelliteBar2, TFT_WIDTH, 55 * scale);
  lv_chart_set_div_line_count(satelliteBar2, 6, 0);
  lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
  satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
  lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
  lv_obj_set_pos(satelliteBar2, 0, 260 * scale);
 
  #ifdef LARGE_SCREEN

  #ifdef AT6558D_GPS
  lv_style_init(&styleRadio);
  lv_style_set_radius(&styleRadio, LV_RADIUS_CIRCLE);
  
  lv_style_init(&styleRadioChk);
  lv_style_set_bg_image_src(&styleRadioChk, NULL);
  
  lv_obj_t *gnssSel = lv_obj_create(satTrackTile);
  lv_obj_set_flex_flow(gnssSel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(gnssSel, TFT_WIDTH, 50);
  lv_obj_set_pos(gnssSel, 0, 330);
  static lv_style_t styleSel;
  lv_style_init(&styleSel);
  lv_style_set_bg_opa(&styleSel, LV_OPA_0);
  lv_style_set_border_opa(&styleSel, LV_OPA_0);
  lv_obj_add_style(gnssSel, &styleSel, LV_PART_MAIN);
  
  lv_obj_t *gps = lv_checkbox_create(gnssSel);
  lv_checkbox_set_text_static(gps, "GPS     ");
  lv_obj_add_flag(gps, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_style(gps, &styleRadio, LV_PART_INDICATOR);
  lv_obj_add_style(gps, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
  
  lv_obj_t *glonass = lv_checkbox_create(gnssSel);
  lv_checkbox_set_text_static(glonass, "GLONASS  ");
  lv_obj_add_flag(glonass, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_style(glonass, &styleRadio, LV_PART_INDICATOR);
  lv_obj_add_style(glonass, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
  
  lv_obj_t *beidou = lv_checkbox_create(gnssSel);
  lv_checkbox_set_text_static(beidou, "BEIDOU");
  lv_obj_add_flag(beidou, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_style(beidou, &styleRadio, LV_PART_INDICATOR);
  lv_obj_add_style(beidou, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
  
  lv_obj_add_state(lv_obj_get_child(gnssSel, 0), LV_STATE_CHECKED);
  
  // GNSS Selection Event
  lv_obj_add_event_cb(gnssSel, activeGnssEvent, LV_EVENT_CLICKED, &activeGnss);
  #endif
  
  #endif

  // Satellite Tracking Event
  lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);
}
