/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Main Screen
 * @version 0.1.8_Alpha
 * @date 2024-08
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

#ifdef LARGE_SCREEN
  int toolBarOffset = 100;
  int toolBarSpace = 60;
#endif
#ifndef LARGE_SCREEN
  int toolBarOffset = 80;
  int toolBarSpace = 50;
#endif

lv_obj_t *compassHeading;
lv_obj_t *compassImg;
lv_obj_t *latitude;
lv_obj_t *longitude;
lv_obj_t *altitude;
lv_obj_t *speedLabel;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *btnFullScreen;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;


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
 * @brief Edit Screen Event (drag widgets)
 *
 * @param event
 */
void editScreen(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  
  if (code == LV_EVENT_VALUE_CHANGED)
    canMoveWidget = !canMoveWidget;
}

/**
 * @brief Unselect widget
 *
 * @param event
 */
void unselectWidget(lv_event_t *event)
{
  if (canMoveWidget)
  {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);
    if (widgetSelected)
    {
      objUnselect(obj);
      lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
      widgetSelected = false;
    }
    isScrolled = true;
  }
}

/**
 * @brief Drag widget event
 *
 * @param event
 */
void dragWidget(lv_event_t *event)
{
  if (canMoveWidget)
  {
    isScrolled = false;
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);
    if (!widgetSelected)
    {
      objSelect(obj);
      lv_obj_clear_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
      widgetSelected = true;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL)
      return;
    
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    
    lv_coord_t x = lv_obj_get_x(obj) + vect.x;
    lv_coord_t y = lv_obj_get_y(obj) + vect.y;
    lv_coord_t width = lv_obj_get_width(obj);
    lv_coord_t height = lv_obj_get_height(obj);
    
    // Limit drag area
    if (x > 0 && y > 0 && (x + width) < TFT_WIDTH && (y + height) < TFT_HEIGHT - 25)
    {
      lv_obj_set_pos(obj, x, y);
      
      char *widget = (char *)lv_event_get_user_data(event);
      saveWidgetPos(widget, x, y);
    }
  }
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

    log_d("Free PSRAM: %d", ESP.getFreePsram());
    log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());

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
            
      deleteMapScrSprites();
      createMapScrSprites();
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
  lv_label_set_text_fmt(distNav,"%d m.", (int)calcDist(getLat(), getLon(), loadWpt.lat, loadWpt.lon));
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
  mapTile = lv_tileview_add_tile(tilesScreen, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
  navTile = lv_tileview_add_tile(tilesScreen, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
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
  
  // Pin drag widget
  static lv_style_t editBtnStyleOff;
  lv_style_init(&editBtnStyleOff);
  lv_style_set_bg_color(&editBtnStyleOff, lv_color_black());
  lv_style_set_text_color(&editBtnStyleOff, lv_color_hex(0x303030));
  static lv_style_t editBtnStyleOn;
  lv_style_init(&editBtnStyleOn);
  lv_style_set_bg_color(&editBtnStyleOn, lv_color_black());
  lv_style_set_text_color(&editBtnStyleOn, lv_color_white());
  lv_obj_t *editScreenBtn = lv_button_create(compassTile);
  lv_obj_add_style(editScreenBtn, &editBtnStyleOff, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(editScreenBtn, &editBtnStyleOn, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_pos(editScreenBtn, 5, 5);
  lv_obj_add_flag(editScreenBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_t *editScreenLbl;
  editScreenLbl = lv_label_create(editScreenBtn);
  lv_label_set_text(editScreenLbl, LV_SYMBOL_EDIT);
  lv_obj_center(editScreenLbl);
  
  // Compass Widget
  lv_obj_t *compassWidget = lv_obj_create(compassTile);
  lv_obj_set_size(compassWidget, 200 * scale, 200 * scale);
  lv_obj_set_pos(compassWidget, compassPosX, compassPosY);
  lv_obj_clear_flag(compassWidget, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *arrowImg = lv_img_create(compassWidget);
  lv_img_set_src(arrowImg, arrowIconFile);
  lv_obj_align(arrowImg, LV_ALIGN_CENTER, 0, -30);
  lv_img_set_zoom(arrowImg,iconScale);
  lv_obj_update_layout(arrowImg);
  LV_IMG_DECLARE(bruj);
  compassImg = lv_img_create(compassWidget);
  lv_img_set_src(compassImg, &bruj);
  lv_img_set_zoom(compassImg,iconScale);
  lv_obj_update_layout(compassImg);
  lv_obj_align_to(compassImg, compassWidget, LV_ALIGN_CENTER, 0, 0);   
  lv_img_set_pivot(compassImg, 100, 100) ;
  compassHeading = lv_label_create(compassWidget);
  lv_obj_set_height(compassHeading,38);
  lv_obj_align(compassHeading, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_text_font(compassHeading, fontVeryLarge, 0);
  lv_label_set_text_static(compassHeading, "---\xC2\xB0");
  objUnselect(compassWidget);
  lv_obj_add_event_cb(compassWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Compass_");
  lv_obj_add_event_cb(compassWidget, unselectWidget, LV_EVENT_RELEASED, NULL);
  
  // Position widget
  lv_obj_t *positionWidget = lv_obj_create(compassTile);
  lv_obj_set_height(positionWidget,40);
  lv_obj_set_pos(positionWidget, coordPosX, coordPosY);
  lv_obj_clear_flag(positionWidget, LV_OBJ_FLAG_SCROLLABLE);
  latitude = lv_label_create(positionWidget);
  lv_obj_set_style_text_font(latitude, fontMedium, 0);
  lv_label_set_text_fmt(latitude, "%s", latFormatString(getLat()));
  longitude = lv_label_create(positionWidget);
  lv_obj_set_style_text_font(longitude, fontMedium, 0);
  lv_label_set_text_fmt(longitude, "%s", lonFormatString(getLon()));
  lv_obj_t *posImg = lv_img_create(positionWidget);
  lv_img_set_src(posImg, positionIconFile);
  lv_img_set_zoom(posImg,iconScale);
  lv_obj_update_layout(latitude);
  lv_obj_update_layout(posImg);
  lv_obj_set_width(positionWidget, lv_obj_get_width(latitude) + 40);
  lv_obj_align(latitude, LV_ALIGN_TOP_LEFT, 15, -12);
  lv_obj_align(longitude, LV_ALIGN_TOP_LEFT, 15, 3);
  lv_obj_align(posImg, LV_ALIGN_TOP_LEFT, -15, -10);
  objUnselect(positionWidget);
  lv_obj_add_event_cb(positionWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Coords_");
  lv_obj_add_event_cb(positionWidget, unselectWidget, LV_EVENT_RELEASED, NULL);
  
  // Altitude widget
  lv_obj_t *altitudeWidget = lv_obj_create(compassTile);
  lv_obj_set_height(altitudeWidget, 40 * scale);
  lv_obj_set_pos(altitudeWidget, altitudePosX, altitudePosY);
  lv_obj_clear_flag(altitudeWidget, LV_OBJ_FLAG_SCROLLABLE);
  altitude = lv_label_create(altitudeWidget);
  lv_obj_set_style_text_font(altitude, fontLargeMedium, 0);
  lv_label_set_text_static(altitude, "0000 m.");
  lv_obj_t *altitImg = lv_img_create(altitudeWidget);
  lv_img_set_src(altitImg, altitudeIconFile);
  lv_img_set_zoom(altitImg,iconScale);
  lv_obj_update_layout(altitude);
  lv_obj_update_layout(altitImg);
  lv_obj_set_width(altitudeWidget, lv_obj_get_width(altitude) + 40);
  lv_obj_align(altitImg, LV_ALIGN_LEFT_MID, -15, 0);
  lv_obj_align(altitude, LV_ALIGN_CENTER, 10, 0);
  objUnselect(altitudeWidget);
  lv_obj_add_event_cb(altitudeWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Altitude_");
  lv_obj_add_event_cb(altitudeWidget, unselectWidget, LV_EVENT_RELEASED, NULL);
  
  // Speed widget
  lv_obj_t *speedWidget = lv_obj_create(compassTile);
  lv_obj_set_height(speedWidget, 40 * scale);
  lv_obj_set_pos(speedWidget, speedPosX, speedPosY);
  lv_obj_clear_flag(speedWidget, LV_OBJ_FLAG_SCROLLABLE);
  speedLabel = lv_label_create(speedWidget);
  lv_obj_set_style_text_font(speedLabel, fontLargeMedium, 0);
  lv_label_set_text_static(speedLabel, "   0 Km/h");
  lv_obj_t *speedImg = lv_img_create(speedWidget);
  lv_img_set_src(speedImg, speedIconFile);
  lv_img_set_zoom(speedImg,iconScale);
  lv_obj_update_layout(speedLabel);
  lv_obj_update_layout(speedImg);
  lv_obj_set_width(speedWidget, lv_obj_get_width(speedLabel) + 40);
  lv_obj_align(speedImg, LV_ALIGN_LEFT_MID, -10, 0);
  lv_obj_align(speedLabel, LV_ALIGN_CENTER, 10, 0);
  objUnselect(speedWidget);
  lv_obj_add_event_cb(speedWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Speed_");
  lv_obj_add_event_cb(speedWidget, unselectWidget, LV_EVENT_RELEASED, NULL);
  
  // Compass Tile Events
  lv_obj_add_event_cb(compassHeading, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(latitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(longitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(altitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(speedLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(editScreenBtn, editScreen, LV_EVENT_ALL, NULL);
 
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
  lv_label_set_text_fmt(nameNav, "%s","");
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
