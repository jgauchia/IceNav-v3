/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Device Config
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *devconfigOptions;

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
    devconfigOptions = lv_obj_create(devconfigScreen);
    lv_obj_set_size(devconfigOptions, TFT_WIDTH, TFT_HEIGHT - 60);

    lv_obj_t *but_label;

    // Back button
    lv_obj_t *back_but = lv_btn_create(devconfigScreen);
    lv_obj_set_size(back_but, TFT_WIDTH - 30, 40);
    but_label = lv_label_create(back_but);
    lv_obj_set_style_text_font(but_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(but_label, "Back");
    lv_obj_center(but_label);
    lv_obj_align(back_but, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back_but, device_conf_back, LV_EVENT_CLICKED, NULL);
}