/**
 * @file mapSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Map Settings screen
 * @version 0.2.1
 * @date 2025-05
 */

#include "mapSettingsScr.hpp"

lv_obj_t *mapSettingsScreen; // Map Settings Screen

extern Maps mapView;

/**
 * @brief Map Settings Events
 *
 * @param event
 */
static void mapSettingsEvents(lv_event_t *event)
{
  lv_obj_t *obj = lv_event_get_target_obj(event);
  lv_event_code_t code = lv_event_get_code(event);

  if (obj == btnBack)
  {
    if (needReboot)
    {   
      cfg.saveBool(PKEYS::KMAP_VECTOR, mapSet.vectorMap);
      cfg.saveUInt(PKEYS::KDEF_ZOOM, defaultZoom);
      lv_obj_delete(mapSettingsScreen);
      showRestartScr();
    }
    else
      lv_screen_load(settingsScreen);
  }

  if (obj == zoomBtnUp)
  {
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
      lv_spinbox_increment(zoomLevel);
      defaultZoom = (uint8_t)lv_spinbox_get_value(zoomLevel);
      zoom = defaultZoom;
      mapView.isPosMoved = true;
      cfg.saveUInt(PKEYS::KDEF_ZOOM, defaultZoom);
    }
  }

  if (obj == zoomBtnDown)
  {
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
      lv_spinbox_decrement(zoomLevel);
      defaultZoom = (uint8_t)lv_spinbox_get_value(zoomLevel);
      zoom = defaultZoom;
      mapView.isPosMoved = true;
      cfg.saveUInt(PKEYS::KDEF_ZOOM, defaultZoom);
    }
  }

  if (obj == mapType)
  {
    mapSet.vectorMap = lv_obj_has_state(obj, LV_STATE_CHECKED);
    if (!mapSet.vectorMap)
    {
      minZoom = 6;
      maxZoom = 17;
    }
    else
    {
      minZoom = 1;
      maxZoom = 4;
    }
    lv_spinbox_set_range(zoomLevel, minZoom, maxZoom);
    defaultZoom = (uint8_t)lv_spinbox_get_value(zoomLevel);
    zoom = defaultZoom;
    needReboot = true;
  }

  if (obj == mapSwitch)
  {
    mapSet.mapRotationComp = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_ROT_MODE, mapSet.mapRotationComp);
  }

  if (obj == checkCompass)
  {
    mapSet.showMapCompass = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_COMPASS, mapSet.showMapCompass);
  }

  if (obj == checkCompassRot)
  {
    mapSet.compassRotation = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_COMP_ROT, mapSet.compassRotation);
  }

  if (obj == checkSpeed)
  {
    mapSet.showMapSpeed = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_SPEED, mapSet.showMapSpeed);
  }

  if (obj == checkScale)
  {
    mapSet.showMapScale = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_SCALE, mapSet.showMapScale);
  }

  if (obj == checkFullScreen)
  {
    mapSet.mapFullScreen = lv_obj_has_state(obj, LV_STATE_CHECKED);
    cfg.saveBool(PKEYS::KMAP_MODE, mapSet.mapFullScreen);
    needReboot = true;
  }
}

/**
 * @brief Create Map Settings screen
 *
 */
void createMapSettingsScr()
{
  // Map Settings Screen
  mapSettingsScreen = lv_obj_create(NULL);
  mapSettingsOptions = lv_list_create(mapSettingsScreen);
  lv_obj_set_size(mapSettingsOptions, TFT_WIDTH, TFT_HEIGHT - 60);

  lv_obj_t *label;

  // Map Type
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Map Type\nRENDER/VECTOR");
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  lv_obj_set_style_text_font(list, fontOptions, 0);
  mapType = lv_switch_create(list);
  label = lv_label_create(mapType);
  lv_label_set_text_static(label, "V   R");
  lv_obj_center(label);
  if (mapSet.vectorMap)
    lv_obj_add_state(mapType, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(mapType, LV_STATE_CHECKED);
  lv_obj_align_to(mapType, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(mapType, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Map Rotation
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Map Rotation Mode\nHEADING/COMPASS");
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  lv_obj_set_style_text_font(list, fontOptions, 0);
  mapSwitch = lv_switch_create(list);
  label = lv_label_create(mapSwitch);
  lv_label_set_text_static(label, "C   H");
  lv_obj_center(label);
  if (mapSet.mapRotationComp)
    lv_obj_add_state(mapSwitch, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(mapSwitch, LV_STATE_CHECKED);
  lv_obj_align_to(mapSwitch, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(mapSwitch, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Default zoom level
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Default\nZoom Level");
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  lv_obj_set_style_text_font(list, fontOptions, 0);
  
  zoomBtnUp = lv_btn_create(list);
  lv_obj_set_size(zoomBtnUp, 40 * scale, 40 * scale);
  lv_obj_align_to(zoomBtnUp, list, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_image_src(zoomBtnUp, LV_SYMBOL_PLUS, 0);
  lv_obj_add_event_cb(zoomBtnUp, mapSettingsEvents, LV_EVENT_ALL, NULL);

  zoomLevel = lv_spinbox_create(list);
  lv_spinbox_set_range(zoomLevel, minZoom, maxZoom);
  lv_obj_set_width(zoomLevel, 40 * scale);
  lv_obj_clear_flag(zoomLevel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_text_font(zoomLevel, fontLarge, 0);
  lv_spinbox_set_value(zoomLevel, defaultZoom);
  lv_spinbox_set_digit_format(zoomLevel, 2, 0);
  lv_obj_align_to(zoomLevel, list, LV_ALIGN_RIGHT_MID, 0, 0);
  objHideCursor(zoomLevel);

  zoomBtnDown = lv_btn_create(list);
  lv_obj_set_size(zoomBtnDown, 40 * scale, 40 * scale);
  lv_obj_align_to(zoomBtnDown, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_image_src(zoomBtnDown, LV_SYMBOL_MINUS, 0);
  lv_obj_add_event_cb(zoomBtnDown, mapSettingsEvents, LV_EVENT_ALL, NULL);

  // Show Full Screen Map
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Full Screen Map");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  checkFullScreen = lv_checkbox_create(list);
  lv_obj_align_to(checkFullScreen, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_checkbox_set_text(checkFullScreen, " ");
  lv_obj_add_state(checkFullScreen, mapSet.mapFullScreen);
  lv_obj_add_event_cb(checkFullScreen, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Show Compass
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Compass");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  checkCompass = lv_checkbox_create(list);
  lv_obj_align_to(checkCompass, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_checkbox_set_text(checkCompass, " ");
  lv_obj_add_state(checkCompass, mapSet.showMapCompass);
  lv_obj_add_event_cb(checkCompass, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Compass Rotation
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Compass Rotation");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  checkCompassRot = lv_checkbox_create(list);
  lv_obj_align_to(checkCompassRot, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_checkbox_set_text(checkCompassRot, " ");
  lv_obj_add_state(checkCompassRot, mapSet.compassRotation);
  lv_obj_add_event_cb(checkCompassRot, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Show Speed
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Speed");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  checkSpeed = lv_checkbox_create(list);
  lv_obj_align_to(checkSpeed, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_checkbox_set_text(checkSpeed, " ");
  lv_obj_add_state(checkSpeed, mapSet.showMapSpeed);
  lv_obj_add_event_cb(checkSpeed, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Show Map Scale
  list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Map Scale");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  checkScale = lv_checkbox_create(list);
  lv_obj_align_to(checkScale, list, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_checkbox_set_text(checkScale, " ");
  lv_obj_add_state(checkScale, mapSet.showMapScale);
  lv_obj_add_event_cb(checkScale, mapSettingsEvents, LV_EVENT_VALUE_CHANGED, NULL);

  // Back button
  btnBack = lv_btn_create(mapSettingsScreen);
  lv_obj_set_size(btnBack, TFT_WIDTH - 30, 40 * scale);
  label = lv_label_create(btnBack);
  lv_obj_set_style_text_font(label, fontLarge, 0);
  lv_label_set_text_static(label, "Back");
  lv_obj_center(label);
  lv_obj_align(btnBack, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_add_event_cb(btnBack, mapSettingsEvents, LV_EVENT_CLICKED, NULL);
}
