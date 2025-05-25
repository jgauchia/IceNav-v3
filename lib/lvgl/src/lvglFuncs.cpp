/**
 * @file lvglFuncs.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL custom functions
 * @version 0.2.1
 * @date 2025-05
 */

#include "lvglFuncs.hpp"

/**
 * @brief Custom LVGL function to hide cursor
 *
 * @param LVGL object
 */
void objHideCursor(_lv_obj_t *obj)
{
  static lv_style_t style1;
  lv_style_init(&style1);
  lv_style_set_bg_opa(&style1, LV_OPA_TRANSP);
  lv_style_set_text_opa(&style1, LV_OPA_TRANSP);
  lv_obj_add_style(obj, &style1, LV_PART_CURSOR);

  static lv_style_t style2;
  lv_style_init(&style2);
  lv_style_set_bg_opa(&style2, LV_OPA_100);
  lv_style_set_text_opa(&style2, LV_OPA_100);
  lv_obj_add_style(obj, &style2, LV_PART_CURSOR | LV_STATE_FOCUS_KEY);
  lv_obj_add_style(obj, &style2, LV_PART_CURSOR | LV_STATE_FOCUSED);
}

/**
 * @brief Custom LVGL function to select widget
 *
 * @param obj
 */
void objSelect(_lv_obj_t *obj)
{
  static lv_style_t styleWidget;
  lv_style_init(&styleWidget);
  lv_style_set_bg_color(&styleWidget, lv_color_hex(0xB8B8B8));
  lv_style_set_bg_opa(&styleWidget, LV_OPA_20);
  lv_style_set_border_opa(&styleWidget, LV_OPA_100);
  lv_obj_add_style(obj, &styleWidget, LV_PART_MAIN);
}

/**
 * @brief Custom LVGL function to unselect widget
 * 
 * @param obj 
 */
void objUnselect(_lv_obj_t *obj)
{
  static lv_style_t styleWidget;
  lv_style_init(&styleWidget);
  lv_style_set_bg_color(&styleWidget, lv_color_black());
  lv_style_set_bg_opa(&styleWidget, LV_OPA_0);
  lv_style_set_border_opa(&styleWidget, LV_OPA_0);
  lv_obj_add_style(obj, &styleWidget, LV_PART_MAIN);
}

/**
 * @brief Restart timer callback
 * 
 * @param timer 
 */
void restartTimerCb(lv_timer_t *timer)
{
  if (lv_timer_get_idle() != 0)
    ESP.restart();
}

/**
 * @brief Show restart Screen
 * 
 */
void showRestartScr()
{
  lv_obj_t *restartScr = lv_obj_create(NULL);
  lv_obj_set_size(restartScr, TFT_WIDTH, TFT_HEIGHT);
  lv_obj_t *restartMsg = lv_msgbox_create(restartScr);
  lv_obj_set_width(restartMsg,TFT_WIDTH - 20);
  lv_obj_set_align(restartMsg,LV_ALIGN_CENTER);
  lv_obj_set_style_text_font(restartMsg, fontDefault, 0);
  lv_obj_t *labelText = lv_msgbox_get_content(restartMsg);
  lv_obj_set_style_text_align(labelText, LV_TEXT_ALIGN_CENTER, 0);
  lv_msgbox_add_text(restartMsg, LV_SYMBOL_WARNING " This device will restart shortly");
  lv_screen_load(restartScr);
  lv_timer_t *restartTimer;
  restartTimer = lv_timer_create(restartTimerCb, 3000, NULL);
  lv_timer_reset(restartTimer);
}
