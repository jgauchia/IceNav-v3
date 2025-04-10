/**
 * @file waypointScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Waypoint Screen
 * @version 0.2.0
 * @date 2025-04
 */

#include "waypointScr.hpp"

extern Maps mapView;
extern Gps gps;
extern wayPoint loadWpt;
extern wayPoint addWpt;

lv_obj_t *waypointScreen;  // Add Waypoint Screen
lv_obj_t *waypointName;
lv_obj_t *lat;
lv_obj_t *lon;
bool isScreenRotated = false;

/**
 * @brief Waypoint Screen event
 *
 * @param event
 */
static void waypointScreenEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *fileName = (lv_obj_t *)lv_event_get_target(event);

  #ifdef TDECK_ESP32S3
    if (code == LV_EVENT_KEY)
    {
      createGpxFile(wptFile);
      GPXParser gpx(wptFile);
      if ( lv_indev_get_key(lv_indev_active()) == 13 ) // Enter Key
      {    
        switch (wptAction)
        {
          case WPT_ADD:
            addWpt.name = (char *)lv_textarea_get_text(fileName);
            if (strcmp(addWpt.name,"") != 0)
              gpx.addWaypoint(addWpt);
            break;
          case WPT_EDIT:
            char *newName = (char *)lv_textarea_get_text(fileName);
            if (strcmp(loadWpt.name, newName) != 0)
              gpx.editWaypointName(loadWpt.name, newName);
            break;
        }

        isMainScreen = true;
        mapView.redrawMap = true;
        wptAction = WPT_NONE;
        lv_refr_now(display);
        loadMainScreen();
      }

      if ( lv_indev_get_key(lv_indev_active()) == 35 ) // # Key (ESCAPE)
      { 
        isMainScreen = true;
        mapView.redrawMap = true;
        wptAction = WPT_NONE;
        lv_refr_now(display);
        loadMainScreen();
      }
    }
  #endif

  if (code == LV_EVENT_READY)
  {
    createGpxFile(wptFile);
    GPXParser gpx(wptFile);
    if (lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_270)
    {
      tft.setRotation(0);
      lv_display_set_rotation(display,LV_DISPLAY_ROTATION_0);
    }
    
    switch (wptAction)
    {
      case WPT_ADD:
        addWpt.name = (char *)lv_textarea_get_text(fileName);
        if (strcmp(addWpt.name,"") != 0)
          gpx.addWaypoint(addWpt);
        break;
      case WPT_EDIT:
        char *newName = (char *)lv_textarea_get_text(fileName);
        if (strcmp(loadWpt.name, newName) != 0)
          gpx.editWaypointName(loadWpt.name, newName);
        break;
    }

    isMainScreen = true;
    mapView.redrawMap = true;
    wptAction = WPT_NONE;
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
    wptAction = WPT_NONE;
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
  lv_obj_set_width(waypointName, tft.width() -10);
  lv_refr_now(display);
}

/**
 * @brief Waypoint Name event
 *
 * @param event
 */
static void waypointNameEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  if(code == LV_EVENT_CLICKED)
  {
    createGpxFile(wptFile);
    GPXParser gpx(wptFile);
    switch (wptAction)
    {
      case WPT_ADD:
        addWpt.name = (char *)lv_textarea_get_text(waypointName);
        if (strcmp(addWpt.name,"") != 0)
          gpx.addWaypoint(addWpt);
        break;
      case WPT_EDIT:
        char *newName = (char *)lv_textarea_get_text(waypointName);
        if (strcmp(loadWpt.name, newName) != 0)
          gpx.editWaypointName(loadWpt.name, newName);
        break;
    }

    isMainScreen = true;
    mapView.redrawMap = true;
    wptAction = WPT_NONE;
    lv_refr_now(display);
    loadMainScreen();
  }
}

/**
 * @brief Update current waypoint position to add
 *
 */
void updateWaypointPos()
{
  // time_t tUTCwpt = time(NULL);
  // struct tm UTCwpt_tm;
  // struct tm *tmUTCwpt = gmtime_r(&tUTCwpt, &UTCwpt_tm);

  switch (wptAction)
  {
    case WPT_ADD:
      addWpt.lat = gps.gpsData.latitude;
      addWpt.lon = gps.gpsData.longitude;
      addWpt.ele = gps.gpsData.altitude;
      addWpt.sat = gps.gpsData.satellites;
      addWpt.hdop = gps.gpsData.hdop;
      addWpt.pdop = gps.gpsData.pdop;
      addWpt.vdop = gps.gpsData.vdop;
      lv_label_set_text_static(lat, latFormatString(addWpt.lat));
      lv_label_set_text_static(lon, lonFormatString(addWpt.lon));
      break;
    case WPT_EDIT:
      lv_label_set_text_static(lat, latFormatString(loadWpt.lat));
      lv_label_set_text_static(lon, lonFormatString(loadWpt.lon));
      break;
    default:
      break;
  }
}

/**
 * @brief Create Waypoint screen
 *
 */
void createWaypointScreen()
{

  waypointScreen = lv_obj_create(NULL);

  waypointName = lv_textarea_create(waypointScreen);
  lv_textarea_set_one_line(waypointName, true);
  lv_obj_align(waypointName, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_width(waypointName, tft.width() - 10);
  lv_obj_add_state(waypointName, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(waypointName, waypointScreenEvent, LV_EVENT_ALL, waypointScreen);
  #ifndef TDECK_ESP32S3
    lv_obj_t *keyboard = lv_keyboard_create(waypointScreen);
    lv_keyboard_set_mode(keyboard,LV_KEYBOARD_MODE_TEXT_UPPER);
    lv_keyboard_set_textarea(keyboard, waypointName);
  #endif

  #ifdef TDECK_ESP32S3
    lv_group_add_obj(scrGroup, waypointName);
    lv_group_focus_obj(waypointName);
  #endif

  #ifndef TDECK_ESP32S3
    // Rotate Screen button
    static lv_style_t editBtnStyleOn;
    lv_style_init(&editBtnStyleOn);
    lv_style_set_bg_color(&editBtnStyleOn, lv_color_black());
    lv_style_set_text_color(&editBtnStyleOn, lv_color_white());
    lv_obj_t *rotateScreenBtn = lv_button_create(waypointScreen); 
    lv_obj_add_style(rotateScreenBtn, &editBtnStyleOn, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(rotateScreenBtn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_flag(rotateScreenBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rotateScreenBtn, rotateScreen, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(rotateScreenBtn, 40, 35);
    lv_obj_t *rotateScreenLbl = lv_label_create(rotateScreenBtn);
    lv_label_set_text(rotateScreenLbl, LV_SYMBOL_LOOP);
    lv_obj_center(rotateScreenLbl);
  #endif

  lv_obj_t* labelWpt;
  labelWpt = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(labelWpt, fontOptions, 0);
  lv_label_set_text_static(labelWpt, LV_SYMBOL_LEFT " Waypoint Name:");
  lv_obj_center(labelWpt);
  lv_obj_align(labelWpt,LV_ALIGN_TOP_LEFT,10,10);
  lv_obj_add_flag(labelWpt, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(labelWpt, waypointNameEvent, LV_EVENT_ALL, NULL);

  lv_obj_t* label;
  label = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lat:");
  lv_obj_set_pos(label, 10, 90);

  label = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lon:");
  lv_obj_set_pos(label, 10, 120);

  lat = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(lat, fontOptions, 0);
  lv_label_set_text_static(lat, latFormatString(addWpt.lat));
  lv_obj_set_pos(lat, 60, 90);
  
  lon = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(lon, fontOptions, 0);
  lv_label_set_text_static(lon, lonFormatString(addWpt.lon));
  lv_obj_set_pos(lon, 60, 120);
}
