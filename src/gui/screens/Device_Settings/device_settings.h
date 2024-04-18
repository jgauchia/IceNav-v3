/**
 * @file deviceSettings.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Device Settings
 * @version 0.1.8
 * @date 2024-04
 */

static lv_obj_t *deviceSettingsOptions;

/**
 * @brief Device Settings events include
 *
 */
#include "gui/screens/Device_Settings/events/device_settings.h"

/**
 * @brief Create Device Settings screen
 *
 */
static void createDeviceSettingsScr()
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
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_OUT_LEFT_BOTTOM);
    dropdown = lv_dropdown_create(list);
    lv_dropdown_set_options(dropdown, "4800\n9600\n19200\n38400");
    lv_dropdown_set_selected(dropdown, gpsBaud);
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(dropdown, setGpsSpeed, LV_EVENT_VALUE_CHANGED, NULL);

    // GPS Update rate
    list = lv_list_add_btn(deviceSettingsOptions, NULL, "GPS\nUpdate rate");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    dropdown = lv_dropdown_create(list);
    lv_dropdown_set_options(dropdown, "1 Hz\n2 Hz\n4 Hz\n5 Hz\n10 Hz");
    lv_dropdown_set_selected(dropdown, gpsUpdate);
#ifndef AT6558D_GPS
    lv_obj_set_style_text_color(list, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_add_state(list, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(dropdown, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_add_state(dropdown, LV_STATE_DISABLED);
#endif
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(dropdown, setGpsUpdateRate, LV_EVENT_VALUE_CHANGED, NULL);

    // Back button
    btn = lv_btn_create(deviceSettingsScreen);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(label, "Back");
    lv_obj_center(label);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, deviceSettingsBack, LV_EVENT_CLICKED, NULL);
}