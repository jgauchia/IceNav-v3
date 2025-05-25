/**
 * @file deviceSettingsScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Device Settings Screen
 * @version 0.2.1
 * @date 2025-05
 */

#include "deviceSettingsScr.hpp"

lv_obj_t *deviceSettingsScreen; // Device Settings Screen

/**
 * @brief Device Settings Events
 * 
 * @param event 
 */
static void deviceSettingsEvent(lv_event_t *event)
{
  lv_obj_t *obj = (lv_obj_t*)lv_event_get_target(event);
  char *option = (char *)lv_event_get_user_data(event);
  if (strcmp(option,"speed") == 0)
  {
    gpsBaud = lv_dropdown_get_selected(obj);
    saveGPSBaud(gpsBaud);
  }
  if (strcmp(option,"rate") == 0)
  {
    gpsUpdate = lv_dropdown_get_selected(obj);
    saveGPSUpdateRate(gpsUpdate);
  }
  if (strcmp(option, "back") == 0)
  {
    cfg.saveUInt(PKEYS::KDEF_BRIGT, defBright);
    lv_screen_load(settingsScreen);
  }
}

/**
 * @brief Brightness callback
 *
 * @param e
 */
static void brightnessEvent(lv_event_t *e)
{
    lv_obj_t *obj =(lv_obj_t*) lv_event_get_target(e);
    defBright =  lv_slider_get_value(obj);
    tft.setBrightness(defBright);
}

/**
 * @brief Upgrade firmware event
 * 
 * @param event 
 */
static void upgradeEvent(lv_event_t *event)
{
  createMsgUpgrade();
  lv_screen_load(msgUpgrade);
  if (!checkFileUpgrade())
  {
    lv_label_set_text_static(msgUprgdText, LV_SYMBOL_WARNING " No Firmware found!");
  }
  else
  {
    lv_label_set_text_static(msgUprgdText, LV_SYMBOL_WARNING " Firmware found!");
    lv_obj_clear_flag(btnMsgUpgrade,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(contMeter,LV_OBJ_FLAG_HIDDEN);
  }
}

/**
 * @brief Create brightness text
 *
 */
static lv_obj_t *createBrightText(lv_obj_t *parent, const char *icon, const char *txt)
{
  lv_obj_t *obj = lv_menu_cont_create(parent);

  lv_obj_t *img = NULL;
  lv_obj_t *label = NULL;

  if (icon) 
  {
    img = lv_img_create(obj);
    lv_img_set_src(img, icon);
  }

  if (txt) 
  {
    label = lv_label_create(obj);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  if (icon && txt)
  {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

/**
 * @brief Create brightness slider
 *
 */
static lv_obj_t *createBrightSlider(lv_obj_t *parent, const char *icon, const char *txt, int32_t min, int32_t max,
                               int32_t val, lv_event_cb_t cb, lv_event_code_t filter)
{
  lv_obj_t *obj = createBrightText(parent, icon, txt);

  lv_obj_t *slider = lv_slider_create(obj);
  lv_obj_set_width(slider,TFT_WIDTH - 80);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (cb != NULL) 
    lv_obj_add_event_cb(slider, cb, filter, NULL);


  if (icon == NULL) 
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

  return slider;
}

/**
 * @brief Create Device Settings screen
 *
 */
void createDeviceSettingsScr()
{
  // Device Settings Screen
  deviceSettingsScreen = lv_obj_create(NULL);
  deviceSettingsOptions = lv_list_create(deviceSettingsScreen);
  lv_obj_set_size(deviceSettingsOptions, TFT_WIDTH, TFT_HEIGHT - 60);
  
  lv_obj_t *label;
  lv_obj_t *list;
  lv_obj_t *btn;
  lv_obj_t *dropdown;
  
  // GPS Speed
  list = lv_list_add_btn(deviceSettingsOptions, NULL, "GPS\nSpeed");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_OUT_LEFT_BOTTOM);
  dropdown = lv_dropdown_create(list);
  lv_dropdown_set_options(dropdown, "4800\n9600\n19200\nAUTO");
  lv_dropdown_set_selected(dropdown, gpsBaud);
  lv_obj_t* item = lv_dropdown_get_list(dropdown);
  lv_obj_set_style_bg_color(item, lv_color_hex(objectColor), LV_PART_SELECTED | LV_STATE_CHECKED);
  lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  lv_obj_set_width(dropdown,TFT_WIDTH / 3);
  lv_obj_add_event_cb(dropdown, deviceSettingsEvent, LV_EVENT_VALUE_CHANGED, (char*)"speed");
  
  // GPS Update rate
  list = lv_list_add_btn(deviceSettingsOptions, NULL, "GPS\nUpdate rate");
  lv_obj_set_style_text_font(list, fontOptions, 0);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
  dropdown = lv_dropdown_create(list);
  lv_dropdown_set_options(dropdown, "1 Hz\n2 Hz\n4 Hz\n5 Hz\n10 Hz");
  lv_dropdown_set_selected(dropdown, gpsUpdate);
  item = lv_dropdown_get_list(dropdown);
  lv_obj_set_style_bg_color(item, lv_color_hex(objectColor), LV_PART_SELECTED | LV_STATE_CHECKED);
  #ifndef AT6558D_GPS
    lv_obj_set_style_text_color(list, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_add_state(list, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(dropdown, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_add_state(dropdown, LV_STATE_DISABLED);
  #endif
  lv_obj_set_width(dropdown,TFT_WIDTH / 3);
  lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(dropdown, deviceSettingsEvent, LV_EVENT_VALUE_CHANGED, (char*)"rate");

  // Upgrade button
  list = lv_list_add_btn(deviceSettingsOptions, NULL, NULL);
  btn = lv_btn_create(list);
  lv_obj_set_size(btn, TFT_WIDTH - 45, 40 * scale);
  label = lv_label_create(btn);
  lv_obj_set_style_text_font(label, fontLarge, 0);
  lv_label_set_text_static(label, "Firmware Upgrade");
  lv_obj_center(label);
  lv_obj_add_event_cb(btn, upgradeEvent, LV_EVENT_CLICKED, NULL);

  // Brightness Slider
  createBrightSlider(deviceSettingsOptions, LV_SYMBOL_SETTINGS, "Brightness", 5, 255, defBright, brightnessEvent, LV_EVENT_VALUE_CHANGED);
  
   // Back button
  btn = lv_btn_create(deviceSettingsScreen);
  lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
  label = lv_label_create(btn);
  lv_obj_set_style_text_font(label, fontLarge, 0);
  lv_label_set_text_static(label, "Back");
  lv_obj_center(label);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_add_event_cb(btn, deviceSettingsEvent, LV_EVENT_CLICKED, (char*)"back");
}
