/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Button bar screen
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *buttonBar;

/**
 * @brief Main screen events include
 *
 */
#include "gui/screens/Button_Bar/events/button_bar.h"

/**
 * @brief Create button bar screen
 *
 */
void create_button_bar_scr()
{
    // Button Bar
    buttonBar = lv_obj_create(mainScreen);
    lv_obj_set_size(buttonBar, TFT_WIDTH, 68);
    lv_obj_set_pos(buttonBar, 0, TFT_HEIGHT - 80);
    lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t style_bar;
    lv_style_init(&style_bar);
    lv_style_set_bg_opa(&style_bar, LV_OPA_0);
    lv_style_set_border_opa(&style_bar, LV_OPA_0);
    lv_obj_add_style(buttonBar, &style_bar, LV_PART_MAIN);

    // Settings Button
    lv_obj_t *settingsBtn = lv_img_create(buttonBar);
    lv_img_set_src(settingsBtn, "F:/settings.bin");
    lv_obj_add_flag(settingsBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(settingsBtn, settings, LV_EVENT_PRESSED, NULL);
}

