/**
 * @file settings_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings screen
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *settingsScreen;
static lv_obj_t *settingsButtons;

/**
 * @brief Settings Screen events include
 *
 */
#include "gui/screens/Settings/events/settings_scr.h"

/**
 * @brief Create Settings screen
 *
 */
void create_settings_scr()
{
    // Settings Screen
    settingsScreen = lv_obj_create(NULL);
    settingsButtons = lv_obj_create(settingsScreen);
    lv_obj_set_size(settingsButtons, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_flex_align(settingsButtons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(settingsButtons, 20, 0);
    lv_obj_set_flex_flow(settingsButtons, LV_FLEX_FLOW_COLUMN);
    static lv_style_t style_settings;
    lv_style_init(&style_settings);
    lv_style_set_bg_opa(&style_settings, LV_OPA_0);
    lv_style_set_border_opa(&style_settings, LV_OPA_0);
    lv_obj_add_style(settingsButtons, &style_settings, LV_PART_MAIN);


    lv_obj_t *but_label;

    // Compass Calibration
    lv_obj_t *compass_calib_but = lv_btn_create(settingsButtons);
    lv_obj_set_size(compass_calib_but, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(compass_calib_but);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Compass Calibration");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(compass_calib_but, compass_calib, LV_EVENT_CLICKED, NULL);

    // Touch Calibration
    lv_obj_t *touch_calib_but = lv_btn_create(settingsButtons);
    lv_obj_set_size(touch_calib_but, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(touch_calib_but);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Touch Calibration");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(touch_calib_but, touch_calib, LV_EVENT_CLICKED, NULL);

    // Back button
    lv_obj_t *back_but = lv_btn_create(settingsButtons);
    lv_obj_set_size(back_but, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(back_but);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Back");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(back_but, back, LV_EVENT_CLICKED, NULL);
}