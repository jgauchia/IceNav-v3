/**
 * @file settings_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings screen
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *settingsButtons;

/**
 * @brief Settings Screen events include
 *
 */
#include "gui/screens/Settings_Menu/events/settings_scr.h"

/**
 * @brief Create Settings screen
 *
 */
static void create_settings_scr()
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
    lv_obj_t *btn;

    // Compass Calibration
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(btn);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Compass Calibration");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(btn, compass_calib, LV_EVENT_CLICKED, NULL);

    // Touch Calibration
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(btn);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Touch Calibration");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(btn, touch_calib, LV_EVENT_CLICKED, NULL);

    // Map Settings
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(btn);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Map Settings");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(btn, map_settings, LV_EVENT_CLICKED, NULL);

    // Device Settings
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(btn);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Device Settings");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(btn, device_settings, LV_EVENT_CLICKED, NULL);

    // Back button
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(btn);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Back");
    lv_obj_center(but_label);
    lv_obj_add_event_cb(btn, back, LV_EVENT_CLICKED, NULL);
}