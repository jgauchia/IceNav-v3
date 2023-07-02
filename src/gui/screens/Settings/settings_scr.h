/**
 * @file settings_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings screen
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Settings Screen events include
 *
 */
#include "gui/screens/Settings/events/settings_scr.h"

static lv_obj_t *settingsScreen;

/**
 * @brief Create Settings screen
 *
 */
void create_settings_scr()
{
    // Settings Screen
    settingsScreen = lv_obj_create(NULL);
    lv_obj_set_size(settingsScreen, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_pos(settingsScreen, 0, 0);
    lv_obj_set_flex_flow(settingsScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settingsScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *but_label;

    // Back button
    lv_obj_t *back_but = lv_btn_create(settingsScreen);
    lv_obj_set_size(back_but, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(back_but);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(but_label, "Back");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(back_but, back, LV_EVENT_CLICKED, NULL);
}