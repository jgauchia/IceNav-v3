/**
 * @file buttonBar.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Button Bar 
 * @version 0.1.8
 * @date 2024-06
 */

#include "buttonBar.hpp"
#include "globalGuiDef.h"

bool isWaypointOpt = false;
bool isTrackOpt = false;
bool isOptionLoaded = false;

lv_obj_t *settingsScreen;

/**
 * @brief Button events
 *
 * @param event
 */
void buttonBarEvent(lv_event_t *event)
{
  lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(event);
  lv_obj_t * buttonBar = lv_obj_get_parent(obj);
  if(lv_obj_get_width(buttonBar) > LV_HOR_RES / 2)
  {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, buttonBar);
    lv_anim_set_exec_cb(&a, hideShowAnim);
    lv_anim_set_start_cb(&a, startHideShowAnim);
    lv_anim_set_completed_cb(&a, endHideShowAnim);
    lv_anim_set_values(&a, 256, 0);
    lv_anim_set_duration(&a, 400);
    lv_anim_start(&a);
  }

  char *option = (char *)lv_event_get_user_data(event);
  if (strcmp(option,"waypoint") == 0)
  {
    log_v("Waypoint");
    isMainScreen = false;
    isWaypointOpt = true;
    isTrackOpt = false;
    if (!isOptionLoaded)
    {
      isOptionLoaded = true;
      loadOptions();
    }
  }
  if (strcmp(option,"track") == 0)
  {
    log_v("Track");
    isMainScreen = false;
    isTrackOpt = true;
    isWaypointOpt = false;
    if (!isOptionLoaded)
    {
      isOptionLoaded = true;
      loadOptions();
    } 
  }
  if (strcmp(option,"settings") == 0)
  {
    log_v("Settings");
    isMainScreen = false;
    lv_screen_load(settingsScreen);
  }
}

/**
 * @brief Options events
 *
 * @param event
 */
void optionEvent(lv_event_t *event)
{
  char *action = (char *)lv_event_get_user_data(event);
  if (strcmp(action,"save") == 0)
  {
    log_v("Save Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
  }
  if (strcmp(action,"load") == 0)
  {
    log_v("Load Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
  }
  if (strcmp(action,"delete") == 0)
  {
    log_v("Delete Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
  }
}

/**
 * @brief Hide/Show Animation
 *
 * @param event
 */
void hideShowAnim(void * var, int32_t v)
{
  lv_obj_t * obj = (lv_obj_t*)var;
  int32_t max_w = lv_obj_get_width(lv_obj_get_parent(obj)) - LV_DPX(4);
  int32_t w;
  w = lv_map(v, 0, 256, LV_DPX(60) * scaleBut, max_w);
  lv_obj_set_width(obj, w);
}

/**
 * @brief Hide/Show start animation callback
 *
 * @param anim
 */
void startHideShowAnim(lv_anim_t * anim)
{
 isScrolled = false;
}

/**
 * @brief Hide/Show finish animation callback
 *
 * @param anim
 */
void endHideShowAnim(lv_anim_t *anim)
{
  isScrolled = true;
}

/**
 * @brief Hide/Show buttons event
 *
 * @param event
 */
void hideShowEvent(lv_event_t * e)
{
  if(lv_event_get_code(e) == LV_EVENT_CLICKED) 
  {
    lv_obj_t * buttonBar = (lv_obj_t *)lv_event_get_user_data(e);
    if(lv_obj_get_width(buttonBar) < LV_HOR_RES / 2)
    {
      lv_anim_t a;
      lv_anim_init(&a);
      lv_anim_set_var(&a, buttonBar);
      lv_anim_set_start_cb(&a, startHideShowAnim);
      lv_anim_set_completed_cb(&a, endHideShowAnim);
      lv_anim_set_exec_cb(&a, hideShowAnim);
      lv_anim_set_values(&a, 0, 256);
      lv_anim_set_duration(&a, 250);
      lv_anim_start(&a);
    }
    else 
    {
      lv_anim_t a;
      lv_anim_init(&a);
      lv_anim_set_var(&a, buttonBar);
      lv_anim_set_exec_cb(&a, hideShowAnim);
      lv_anim_set_start_cb(&a, startHideShowAnim);
      lv_anim_set_completed_cb(&a, endHideShowAnim);
      lv_anim_set_values(&a, 256, 0);
      lv_anim_set_duration(&a, 250);
      lv_anim_start(&a);
    }
  }
}

/**
 * @brief Create button bar screen
 *
 */
void createButtonBarScr()
{
  // Button Bar
  buttonBar = lv_obj_create(mainScreen);
  lv_obj_remove_style_all(buttonBar);
  lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(buttonBar, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_style_radius(buttonBar, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(buttonBar, lv_color_white(), 0);
  lv_obj_set_style_border_width(buttonBar, 1, 0);
  lv_obj_set_style_border_opa(buttonBar,LV_OPA_20,0);
  lv_obj_add_flag(buttonBar, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_size(buttonBar, 50 * scaleBut, 50 * scaleBut);
  lv_obj_align(buttonBar, LV_ALIGN_BOTTOM_RIGHT, 0,  -LV_DPX(14) );

  lv_obj_t *menuBtn = lv_img_create(mainScreen);
  lv_img_set_src(menuBtn, menuIconFile);
  lv_obj_add_flag(menuBtn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
  lv_img_set_zoom(menuBtn,buttonScale);
  lv_obj_update_layout(menuBtn);
  lv_obj_add_event_cb(menuBtn, hideShowEvent, LV_EVENT_ALL, buttonBar);
  lv_obj_set_size(menuBtn, 48 * scaleBut, 48 * scaleBut);
  lv_obj_align(menuBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -LV_DPX(15));

  lv_obj_t *imgBtn;
  
  // Waypoint Button
  imgBtn = lv_img_create(buttonBar);
  lv_img_set_src(imgBtn, waypointIconFile);
  lv_obj_set_style_img_recolor_opa(imgBtn, 230, 0);
  lv_obj_set_style_img_recolor(imgBtn, lv_color_black(), 0);
  lv_img_set_zoom(imgBtn,buttonScale);
  lv_obj_update_layout(imgBtn);
  lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  //lv_obj_add_event_cb(imgBtn, buttonBarEvent, LV_EVENT_PRESSED, (char*)"waypoint");
  
  // Track Button
  imgBtn = lv_img_create(buttonBar);
  lv_img_set_src(imgBtn, trackIconFile);
  lv_obj_set_style_img_recolor_opa(imgBtn, 230, 0);
  lv_obj_set_style_img_recolor(imgBtn, lv_color_black(), 0);
  lv_img_set_zoom(imgBtn,buttonScale);
  lv_obj_update_layout(imgBtn);
  lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  //lv_obj_add_event_cb(imgBtn, buttonBarEvent, LV_EVENT_PRESSED, (char*)"track");
  
  // Settings Button
  imgBtn = lv_img_create(buttonBar);
  lv_img_set_src(imgBtn, settingsIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_img_set_zoom(imgBtn,buttonScale);
  lv_obj_update_layout(imgBtn);
  lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
  lv_obj_add_event_cb(imgBtn, buttonBarEvent, LV_EVENT_PRESSED, (char*)"settings");
}

/**
 * @brief Load waypoint, track options modal dialog.
 *
 */
void loadOptions()
{
  option = lv_msgbox_create(lv_scr_act());
  if (isWaypointOpt)
  {
    //lv_msgbox_add_title(option,"Waypoint Options");
    //option = lv_msgbox_create(lv_scr_act(), "Waypoint Options", NULL, NULL, true);
  }
  else if (isTrackOpt)
  {
    //lv_msgbox_add_title(option,"Track Options");
    //option = lv_msgbox_create(lv_scr_act(), "Track Options", NULL, NULL, true);
  }
  lv_msgbox_add_close_button(option);
  
  lv_obj_set_size(option, TFT_WIDTH, 128);
  lv_obj_set_pos(option, 0, TFT_HEIGHT - 200);
  lv_obj_clear_flag(option, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_add_event_cb(lv_msgbox_get_close_btn(option), close_option, LV_EVENT_PRESSED, NULL);
  
  lv_obj_t *imgBtn;
  
  // Save Button
  imgBtn = lv_img_create(option);
  lv_img_set_src(imgBtn, saveIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(imgBtn, optionEvent, LV_EVENT_PRESSED, (char*)"save");
  
  // Load Button
  imgBtn = lv_img_create(option);
  lv_img_set_src(imgBtn, loadIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(imgBtn, optionEvent, LV_EVENT_PRESSED, (char*)"load");
  
  // Delete Button
  imgBtn = lv_img_create(option);
  lv_img_set_src(imgBtn, deleteIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(imgBtn, optionEvent, LV_EVENT_PRESSED, (char*)"delete");
}
