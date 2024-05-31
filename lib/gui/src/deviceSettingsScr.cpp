/**
 * @file deviceSettingsScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Device Settings Screen
 * @version 0.1.8
 * @date 2024-05
 */

#include "deviceSettingsScr.hpp"

lv_obj_t *deviceSettingsScreen; // Device Settings Screen

/**
 * @brief Device Settins Events
 * 
 * @param event 
 */
void deviceSettingsEvent(lv_event_t *event)
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
    if (strcmp(option,"back") == 0)
        lv_screen_load(settingsScreen);
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
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_OUT_LEFT_BOTTOM);
    dropdown = lv_dropdown_create(list);
    lv_dropdown_set_options(dropdown, "4800\n9600\n19200\n38400");
    lv_dropdown_set_selected(dropdown, gpsBaud);
    lv_obj_t* item = lv_dropdown_get_list(dropdown);
    lv_obj_set_style_bg_color(item, lv_color_hex(objectColor), LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(dropdown, deviceSettingsEvent, LV_EVENT_VALUE_CHANGED, (char*)"speed");
    
    // GPS Update rate
    list = lv_list_add_btn(deviceSettingsOptions, NULL, "GPS\nUpdate rate");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
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
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(dropdown, deviceSettingsEvent, LV_EVENT_VALUE_CHANGED, (char*)"rate");
    
    // Back button
    btn = lv_btn_create(deviceSettingsScreen);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(label, "Back");
    lv_obj_center(label);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, deviceSettingsEvent, LV_EVENT_CLICKED, (char*)"back");
}
