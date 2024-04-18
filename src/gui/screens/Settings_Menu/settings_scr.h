/**
 * @file settings_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Settings screen
 * @version 0.1.8
 * @date 2024-04
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
static void createSettingsScr()
{
    // Settings Screen
    settingsScreen = lv_obj_create(NULL);
    settingsButtons = lv_obj_create(settingsScreen);
    lv_obj_set_size(settingsButtons, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_flex_align(settingsButtons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(settingsButtons, 20, 0);
    lv_obj_set_flex_flow(settingsButtons, LV_FLEX_FLOW_COLUMN);
    static lv_style_t styleSettings;
    lv_style_init(&styleSettings);
    lv_style_set_bg_opa(&styleSettings, LV_OPA_0);
    lv_style_set_border_opa(&styleSettings, LV_OPA_0);
    lv_obj_add_style(settingsButtons, &styleSettings, LV_PART_MAIN);

    lv_obj_t *btnLabel;
    lv_obj_t *btn;

    // Compass Calibration
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(btnLabel, "Compass Calibration");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, compassCalib, LV_EVENT_CLICKED, NULL);

    // Touch Calibration
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(btnLabel, "Touch Calibration");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, touchCalib, LV_EVENT_CLICKED, NULL);

    // Map Settings
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(btnLabel, "Map Settings");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, mapSettings, LV_EVENT_CLICKED, NULL);

    // Device Settings
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(btnLabel, "Device Settings");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, deviceSettings, LV_EVENT_CLICKED, NULL);

    // Back button
    btn = lv_btn_create(settingsButtons);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(btnLabel, "Back");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, back, LV_EVENT_CLICKED, NULL);
}