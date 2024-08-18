/**
 * @file addWaypointScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Add Waypoint Screen
 * @version 0.1.8
 * @date 2024-06
 */

#include "addWaypointScr.hpp"
#include "core/lv_obj_pos.h"
#include "globalGuiDef.h"
#include "tft.hpp"
#include "addWaypoint.hpp"

lv_obj_t *addWaypointScreen;  // Add Waypoint Screen
lv_obj_t *waypointName;

/**
 * @brief Add Waypoint event
 *
 * @param event
 */
static void addWaypointEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *fileName = (lv_obj_t *)lv_event_get_target(event);

  if (code == LV_EVENT_READY)
  {
    addWpt.name = (char *)lv_textarea_get_text(fileName);
    log_i("Name %s",addWpt.name);
    log_i("Lat %f",addWpt.latitude);
    log_i("Lon %f",addWpt.longitude);
    isMainScreen = true;
    loadMainScreen();
  }

  if (code == LV_EVENT_CANCEL)
  {
    isMainScreen = true;
    loadMainScreen();
  }
}

/**
 * @brief Create Add Waypoint screen
 *
 */
void createAddWaypointScreen()
{
  addWpt.latitude = getLat();
  addWpt.longitude = getLon();
  addWpt.name = "";

  addWaypointScreen = lv_obj_create(NULL);
  lv_obj_t *keyboard = lv_keyboard_create(addWaypointScreen);
  waypointName = lv_textarea_create(addWaypointScreen);
  lv_textarea_set_one_line(waypointName, true);
  lv_obj_align(waypointName, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_width(waypointName, TFT_WIDTH -10);
  lv_obj_add_state(waypointName, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(waypointName, addWaypointEvent, LV_EVENT_ALL, addWaypointScreen);
  lv_keyboard_set_mode(keyboard,LV_KEYBOARD_MODE_TEXT_UPPER);
  lv_keyboard_set_textarea(keyboard, waypointName);

  lv_obj_t* label;
  label = lv_label_create(addWaypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Waypoint Name:");
  lv_obj_center(label);
  lv_obj_align(label,LV_ALIGN_TOP_LEFT,10,10);

  label = lv_label_create(addWaypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lat:");
  lv_obj_set_pos(label, 10, 90);

  label = lv_label_create(addWaypointScreen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lon:");
  lv_obj_set_pos(label, 10, 120);

  lv_obj_t *lat = lv_label_create(addWaypointScreen);
  lv_obj_set_style_text_font(lat, fontOptions, 0);
  lv_label_set_text_static(lat, latFormatString(addWpt.latitude));
  lv_obj_set_pos(lat, 60, 90);
  
  lv_obj_t *lon = lv_label_create(addWaypointScreen);
  lv_obj_set_style_text_font(lon, fontOptions, 0);
  lv_label_set_text_static(lon, lonFormatString(addWpt.longitude));
  lv_obj_set_pos(lon, 60, 120);
}
