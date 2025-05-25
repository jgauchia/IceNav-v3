/**
 * @file gpxDetailScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX Tag detail Screen
 * @version 0.2.1
 * @date 2025-05
 */

#include "gpxDetailScr.hpp"

extern Maps mapView;
extern Gps gps;
extern wayPoint loadWpt;
extern wayPoint addWpt;
String gpxFileFolder;
// bool gpxWaypoint;
// bool gpxTrack;

lv_obj_t *gpxDetailScreen;  
lv_obj_t *gpxTag;
lv_obj_t *gpxTagValue;
lv_obj_t *labelLat;
lv_obj_t *labelLatValue;
lv_obj_t *labelLon;
lv_obj_t *labelLonValue;
bool isScreenRotated = false;

/**
 * @brief GPX Detail Screen event
 *
 * @param event
 */
static void gpxDetailScreenEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *tagName = (lv_obj_t *)lv_event_get_target(event);

  #ifdef TDECK_ESP32S3
    if (code == LV_EVENT_KEY)
    {
      if ( lv_indev_get_key(lv_indev_active()) == 13 ) // Enter Key
      {    
        createWptFile();
        GPXParser gpx;

        switch (gpxAction)
        {
          case WPT_ADD:
            addWpt.name = (char *)lv_textarea_get_text(tagName);
            if (strcmp(addWpt.name,"") != 0)
            {
              gpx.filePath = wptFile;
              gpx.addWaypoint(addWpt);
            }
            break;
          case GPX_EDIT:
            char *newName = (char *)lv_textarea_get_text(tagName);
            if (strcmp(loadWpt.name, newName) != 0)
            {
              gpx.filePath = gpxFileFolder.c_str();
              if (gpxWaypoint)
                gpx.editTagAttrOrElem(gpxWaypointTag, nullptr, gpxNameElem, loadWpt.name, newName);
              if (gpxTrack)
                gpx.editTagAttrOrElem(gpxTrackTag, nullptr, gpxNameElem, loadWpt.name, newName);
            }
            break;
        }

        isMainScreen = true;
        mapView.redrawMap = true;
        gpxAction = WPT_NONE;
        lv_refr_now(display);
        loadMainScreen();
      }

      if ( lv_indev_get_key(lv_indev_active()) == 35 ) // # Key (ESCAPE)
      { 
        isMainScreen = true;
        mapView.redrawMap = true;
        gpxAction = WPT_NONE;
        lv_refr_now(display);
        loadMainScreen();
      }
    }
  #endif

  if (code == LV_EVENT_READY)
  {
    if (lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_270)
    {
      tft.setRotation(0);
      lv_display_set_rotation(display,LV_DISPLAY_ROTATION_0);
    }
    
    createWptFile();
    GPXParser gpx;

    switch (gpxAction)
    {
      case WPT_ADD:
        addWpt.name = (char *)lv_textarea_get_text(tagName);
        if (strcmp(addWpt.name,"") != 0)
        {
          gpx.filePath = wptFile;
          gpx.addWaypoint(addWpt);
        }
        break;
      case GPX_EDIT:
        char *newName = (char *)lv_textarea_get_text(tagName);
        if (strcmp(loadWpt.name, newName) != 0)
        {
          gpx.filePath = gpxFileFolder.c_str();
          if (gpxWaypoint)
            gpx.editTagAttrOrElem(gpxWaypointTag, nullptr, gpxNameElem, loadWpt.name, newName);
          if (gpxTrack)
            gpx.editTagAttrOrElem(gpxTrackTag, nullptr, gpxNameElem, loadWpt.name, newName);
        }
        break;
    }

    isMainScreen = true;
    mapView.redrawMap = true;
    gpxAction = WPT_NONE;
    lv_refr_now(display);
    loadMainScreen();
  }

  if (code == LV_EVENT_CANCEL)
  {
    if (lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_270)
    {
      tft.setRotation(0);
      lv_display_set_rotation(display,LV_DISPLAY_ROTATION_0);
    }
    isMainScreen = true;
    mapView.redrawMap = true;
    gpxAction = WPT_NONE;
    lv_refr_now(display);
    loadMainScreen();
  }
}

/**
 * @brief Rotate Screen event
 *
 * @param event
 */
static void rotateScreen(lv_event_t *event)
{
  isScreenRotated = !isScreenRotated;
  log_v("%d",isScreenRotated);
  if (isScreenRotated)
  {
    tft.setRotation(1);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270); 
  }
  else
  {
    tft.setRotation(0);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
  }
  lv_obj_set_width(gpxTagValue, tft.width() -10);
  lv_refr_now(display);
}

/**
 * @brief GPX Tag Name event
 *
 * @param event
 */
static void gpxTagNameEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  if(code == LV_EVENT_CLICKED)
  {
    createWptFile();
    GPXParser gpx;

    switch (gpxAction)
    {
      case WPT_ADD:
        addWpt.name = (char *)lv_textarea_get_text(gpxTagValue);
        if (strcmp(addWpt.name,"") != 0)
        {
          gpx.filePath = wptFile;
          gpx.addWaypoint(addWpt);
        }
      case GPX_EDIT:
        char *newName = (char *)lv_textarea_get_text(gpxTagValue);
        if (strcmp(loadWpt.name, newName) != 0)
        {
          gpx.filePath = gpxFileFolder.c_str();
          if (gpxWaypoint)
            gpx.editTagAttrOrElem(gpxWaypointTag, nullptr, gpxNameElem, loadWpt.name, newName);
          if (gpxTrack)
            gpx.editTagAttrOrElem(gpxTrackTag, nullptr, gpxNameElem, loadWpt.name, newName);
        }
        break;
    }

    isMainScreen = true;
    mapView.redrawMap = true;
    gpxAction = WPT_NONE;
    lv_refr_now(display);
    loadMainScreen();
  }
}

/**
 * @brief Update current waypoint 
 *
 * @param action add or edit action
 */
void updateWaypoint(uint8_t action)
{
  switch (action)
  {
    case WPT_ADD:
      addWpt.lat = gps.gpsData.latitude;
      addWpt.lon = gps.gpsData.longitude;
      addWpt.ele = gps.gpsData.altitude;
      addWpt.sat = gps.gpsData.satellites;
      addWpt.hdop = gps.gpsData.hdop;
      addWpt.pdop = gps.gpsData.pdop;
      addWpt.vdop = gps.gpsData.vdop;
      lv_label_set_text_static(labelLatValue, latFormatString(addWpt.lat));
      lv_label_set_text_static(labelLonValue, lonFormatString(addWpt.lon));
      break;
    case GPX_EDIT:
      lv_label_set_text_static(labelLatValue, latFormatString(loadWpt.lat));
      lv_label_set_text_static(labelLonValue, lonFormatString(loadWpt.lon));
      break;
    default:
      break;
  }
}

/**
 * @brief Create Waypoint screen
 *
 */
void createGpxDetailScreen()
{
  gpxDetailScreen = lv_obj_create(NULL);

  gpxTagValue = lv_textarea_create(gpxDetailScreen);
  lv_textarea_set_one_line(gpxTagValue, true);
  lv_obj_align(gpxTagValue, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_width(gpxTagValue, tft.width() - 10);
  lv_obj_add_state(gpxTagValue, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(gpxTagValue, gpxDetailScreenEvent, LV_EVENT_ALL, gpxDetailScreen);
  #ifndef TDECK_ESP32S3
    lv_obj_t *keyboard = lv_keyboard_create(gpxDetailScreen);
    lv_keyboard_set_mode(keyboard,LV_KEYBOARD_MODE_TEXT_UPPER);
    lv_keyboard_set_textarea(keyboard, gpxTagValue);
  #endif

  #ifdef TDECK_ESP32S3
    lv_group_add_obj(scrGroup, gpxTagValue);
    lv_group_focus_obj(gpxTagValue);
  #endif

  #ifndef TDECK_ESP32S3
    // Rotate Screen button
    static lv_style_t editBtnStyleOn;
    lv_style_init(&editBtnStyleOn);
    lv_style_set_bg_color(&editBtnStyleOn, lv_color_black());
    lv_style_set_text_color(&editBtnStyleOn, lv_color_white());
    lv_obj_t *rotateScreenBtn = lv_button_create(gpxDetailScreen); 
    lv_obj_add_style(rotateScreenBtn, &editBtnStyleOn, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(rotateScreenBtn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_flag(rotateScreenBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rotateScreenBtn, rotateScreen, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(rotateScreenBtn, 40, 35);
    lv_obj_t *rotateScreenLbl = lv_label_create(rotateScreenBtn);
    lv_label_set_text(rotateScreenLbl, LV_SYMBOL_LOOP);
    lv_obj_center(rotateScreenLbl);
  #endif


  gpxTag = lv_label_create(gpxDetailScreen);
  lv_obj_set_style_text_font(gpxTag, fontOptions, 0);
  lv_label_set_text_static(gpxTag, LV_SYMBOL_LEFT " Waypoint Name:");
  lv_obj_center(gpxTag);
  lv_obj_align(gpxTag,LV_ALIGN_TOP_LEFT,10,10);
  lv_obj_add_flag(gpxTag, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(gpxTag, gpxTagNameEvent, LV_EVENT_ALL, NULL);

  labelLat = lv_label_create(gpxDetailScreen);
  lv_obj_set_style_text_font(labelLat, fontOptions, 0);
  lv_label_set_text_static(labelLat, "Lat:");
  lv_obj_set_pos(labelLat, 10, 90);

  labelLon = lv_label_create(gpxDetailScreen);
  lv_obj_set_style_text_font(labelLon, fontOptions, 0);
  lv_label_set_text_static(labelLon, "Lon:");
  lv_obj_set_pos(labelLon, 10, 120);

  labelLatValue = lv_label_create(gpxDetailScreen);
  lv_obj_set_style_text_font(labelLatValue, fontOptions, 0);
  lv_label_set_text_static(labelLatValue, latFormatString(addWpt.lat));
  lv_obj_set_pos(labelLatValue, 60, 90);
  
  labelLonValue = lv_label_create(gpxDetailScreen);
  lv_obj_set_style_text_font(labelLonValue, fontOptions, 0);
  lv_label_set_text_static(labelLonValue, lonFormatString(addWpt.lon));
  lv_obj_set_pos(labelLonValue, 60, 120);
}
