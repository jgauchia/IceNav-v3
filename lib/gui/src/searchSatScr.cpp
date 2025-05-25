/**
 * @file searchSatScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.2.1
 * @date 2025-05
 */

#include "searchSatScr.hpp"

static unsigned long millisActual = 0;
static bool skipSearch = false;
bool isSearchingSat = true;
extern uint8_t activeTile;
lv_timer_t *mainTimer;        // Main Screen Timer

/**
 * @brief Button events
 *
 * @param event
 */
void buttonEvent(lv_event_t *event)
{
  char *option = (char *)lv_event_get_user_data(event);
  if (strcmp(option,"skip") == 0)
    skipSearch = true;
  if (strcmp(option,"settings") == 0)
    lv_screen_load(settingsScreen); 
  lv_timer_resume(mainTimer);
}

/**
 * @brief Search valid GPS signal
 *
 */
void searchGPS(lv_timer_t *searchTimer)
{ 
  if (isGpsFixed)
  {
    millisActual = millis();
    while (millis() < millisActual + 500)
      ;
    lv_timer_del(searchTimer);
    lv_timer_resume(mainTimer);
    isSearchingSat = false;
    loadMainScreen();
  }

  if (skipSearch)
  {
    lv_timer_del(searchTimer);
    isSearchingSat = false;
    zoom = defaultZoom;
    activeTile = 3;
    lv_tileview_set_tile_by_index(tilesScreen, 3, 0, LV_ANIM_OFF);
    loadMainScreen();
  }
}

/**
 * @brief Create Satellite Search Screen
 *
 */
void createSearchSatScr()
{
  searchTimer = lv_timer_create(searchGPS, 100, NULL);
  lv_timer_ready(searchTimer);
  lv_timer_pause(mainTimer);

  searchSatScreen = lv_obj_create(NULL);

  lv_obj_t *label = lv_label_create(searchSatScreen);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
  lv_label_set_text(label, textSearch);
  lv_obj_set_align(label, LV_ALIGN_CENTER);
  lv_obj_set_y(label, -100);

  lv_obj_t *spinner = lv_spinner_create(searchSatScreen);
  lv_obj_set_size(spinner, 130, 130);
  lv_spinner_set_anim_params(spinner, 2000, 200);
  lv_obj_center(spinner);

  lv_obj_t *satImg = lv_img_create(searchSatScreen);
  lv_img_set_src(satImg, satIconFile);
  lv_obj_set_align(satImg, LV_ALIGN_CENTER);

  // Button Bar
  lv_obj_t *buttonBar = lv_obj_create(searchSatScreen);
  lv_obj_set_size(buttonBar, TFT_WIDTH, 68 * scaleBut);
  lv_obj_set_pos(buttonBar, 0, TFT_HEIGHT - 80 * scaleBut);
  lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_SCROLLABLE);
  static lv_style_t styleBar;
  lv_style_init(&styleBar);
  lv_style_set_bg_opa(&styleBar, LV_OPA_0);
  lv_style_set_border_opa(&styleBar, LV_OPA_0);
  lv_obj_add_style(buttonBar, &styleBar, LV_PART_MAIN);

  lv_obj_t *imgBtn;
 
  // Settings Button
  imgBtn = lv_img_create(buttonBar);
  lv_img_set_src(imgBtn, confIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_img_set_zoom(imgBtn,buttonScale);
  lv_obj_update_layout(imgBtn);
  lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
  lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"settings");
  
  // Skip Button
  imgBtn = lv_img_create(buttonBar);
  lv_img_set_src(imgBtn, skipIconFile);
  lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_img_set_zoom(imgBtn,buttonScale);
  lv_obj_update_layout(imgBtn);
  lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
  lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"skip");
}
