/**
 * @file waypointScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Waypoint Screen
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "waypointScr.hpp"
#include "core/lv_obj_pos.h"
#include "display/lv_display.h"
#include "globalGuiDef.h"
#include "tft.hpp"
#include "addWaypoint.hpp"

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

  if (code == LV_EVENT_READY)
  {
   
    if (lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_270)
    {
      tft.setRotation(0);
      lv_display_set_rotation(display,LV_DISPLAY_ROTATION_0);
    }
    
    switch (wptAction)
    {
      case WPT_ADD:
        addWpt.name = (char *)lv_textarea_get_text(fileName);

        if ( strcmp(addWpt.name,"") != 0)
        {
          openGpxFile(wptFile);
          vTaskDelay(100);
          addWaypointToFile(wptFile,addWpt);
        }
        break;
      case WPT_EDIT:
        char *newName = (char *)lv_textarea_get_text(fileName);

        if ( strcmp(loadWpt.name, newName) != 0)
        {
           editWaypointName(loadWpt.name, newName);
        }
        break;
      // default:
      //   break;
    }

    isMainScreen = true;
    redrawMap = true;
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
    redrawMap = true;
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
 * @brief Update current waypoint postion to add
 *
 */
void updateWaypointPos()
{
  switch (wptAction)
  {
    case WPT_ADD:
      addWpt.lat = getLat();
      addWpt.lon = getLon();
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
  lv_obj_t *keyboard = lv_keyboard_create(waypointScreen);
  waypointName = lv_textarea_create(waypointScreen);
  lv_textarea_set_one_line(waypointName, true);
  lv_obj_align(waypointName, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_width(waypointName, tft.width() - 10);
  lv_obj_add_state(waypointName, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(waypointName, waypointScreenEvent, LV_EVENT_ALL, waypointScreen);
  lv_keyboard_set_mode(keyboard,LV_KEYBOARD_MODE_TEXT_UPPER);
  lv_keyboard_set_textarea(keyboard, waypointName);

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
  lv_obj_t *rotateScreenLbl = lv_label_create(rotateScreenBtn);
  lv_label_set_text(rotateScreenLbl, LV_SYMBOL_LOOP);
  lv_obj_center(rotateScreenLbl);

  lv_obj_t* label;
  label = lv_label_create(waypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Waypoint Name:");
  lv_obj_center(label);
  lv_obj_align(label,LV_ALIGN_TOP_LEFT,10,10);

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
