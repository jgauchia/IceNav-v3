/**
 * @file device_settings.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Device Settings
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *devicesettingsOptions;

/**
 * @brief Device Settings events include
 *
 */
#include "gui/screens/Device_Settings/events/device_settings.h"

/**
 * @brief Create Device Settings screen
 *
 */
static void create_device_settings_scr()
{
    // Device Settings Screen
    devicesettingsScreen = lv_obj_create(NULL);
    devicesettingsOptions = lv_list_create(devicesettingsScreen);
    lv_obj_set_size(devicesettingsOptions, TFT_WIDTH, TFT_HEIGHT - 60);

    lv_obj_t *label;
    lv_obj_t *list;
    lv_obj_t *btn;
    lv_obj_t *dropdown;

    // GPS Speed
    list = lv_list_add_btn(devicesettingsOptions, NULL, "GPS\nSpeed");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_OUT_LEFT_BOTTOM);
    dropdown = lv_dropdown_create(list);
    lv_dropdown_set_options(dropdown, "4800\n9600\n19200\n38400");
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, -40);

    // GPS Update rate
    list = lv_list_add_btn(devicesettingsOptions, NULL, "GPS\nUpdate rate");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    dropdown = lv_dropdown_create(list);
    lv_dropdown_set_options(dropdown, "1 Hz\n2 Hz\n4 Hz\n5 Hz\n10 Hz");
    lv_obj_align_to(dropdown, list, LV_ALIGN_OUT_RIGHT_MID, 0, -40);

    // Back button
    btn = lv_btn_create(devicesettingsScreen);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(label, "Back");
    lv_obj_center(label);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, device_settings_back, LV_EVENT_CLICKED, NULL);
}