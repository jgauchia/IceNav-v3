/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Button bar screen
 * @version 0.1.6
 * @date 2023-06-14
 */

static lv_obj_t *buttonBar;
static lv_obj_t *option;

/**
 * @brief Main screen events include
 *
 */
#include "gui/screens/Button_Bar/events/button_bar.h"
#include "gui/screens/Button_Bar/events/options.h"

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

    lv_obj_t *imgBtn;

    // Waypoint Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, "F:/wpt.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, waypoint, LV_EVENT_PRESSED, NULL);

    // Track Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, "F:/track.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, track, LV_EVENT_PRESSED, NULL);

    // Settings Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, "F:/settings.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, settings, LV_EVENT_PRESSED, NULL);
}

/**
 * @brief Load waypoint, track options modal dialog.
 *
 */
static void load_options()
{
    if (is_waypoint)
        option = lv_msgbox_create(lv_scr_act(), "Waypoint Options", NULL, NULL, true);
    else if (is_track)
        option = lv_msgbox_create(lv_scr_act(), "Track Options", NULL, NULL, true);

    lv_obj_set_size(option, TFT_WIDTH, 128);
    lv_obj_set_pos(option, 0, TFT_HEIGHT - 200);
    lv_obj_clear_flag(option, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(((lv_msgbox_t *)option)->close_btn, close_option, LV_EVENT_PRESSED, NULL);

    lv_obj_t *imgBtn;

    // Save Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, "F:/save.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, save_option, LV_EVENT_PRESSED, NULL);

    // Load Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, "F:/load.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, load_option, LV_EVENT_PRESSED, NULL);

    // Delete Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, "F:/delete.bin");
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, delete_option, LV_EVENT_PRESSED, NULL);
}