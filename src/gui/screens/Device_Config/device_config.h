/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Device Config
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *devconfigOptions;
static lv_obj_t *map_switch;

/**
 * @brief Device Config events include
 *
 */
#include "gui/screens/Device_Config/events/device_config.h"

/**
 * @brief Create a device config screen
 *
 */
static void create_device_config_scr()
{
    // Device Config Screen
    devconfigScreen = lv_obj_create(NULL);
    devconfigOptions = lv_list_create(devconfigScreen);
    lv_obj_set_size(devconfigOptions, TFT_WIDTH, TFT_HEIGHT - 60);

    lv_obj_t *label;

    // Map Rotation
    lv_obj_t *map_rot_cfg = lv_list_add_btn(devconfigOptions, NULL, "Map Rotation\nHEADING/COMPASS");
    lv_obj_set_align(map_rot_cfg, LV_ALIGN_LEFT_MID);
    map_switch = lv_switch_create(map_rot_cfg);
    if (map_rotation)
        lv_obj_add_state(map_switch, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(map_switch, LV_STATE_CHECKED);
    lv_obj_align_to(map_switch, map_rot_cfg, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(map_switch, configure_map_rotation, LV_EVENT_VALUE_CHANGED, NULL);

    // Back button
    lv_obj_t *back_but = lv_btn_create(devconfigScreen);
    lv_obj_set_size(back_but, TFT_WIDTH - 30, 40);
    label = lv_label_create(back_but);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(label, "Back");
    lv_obj_center(label);
    lv_obj_align(back_but, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back_but, device_conf_back, LV_EVENT_CLICKED, NULL);
}