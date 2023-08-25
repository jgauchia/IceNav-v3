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

    // Waypoint Button
    lv_obj_t *waypointBtn = lv_img_create(buttonBar);
    lv_img_set_src(waypointBtn, "F:/wpt.bin");
    lv_obj_add_flag(waypointBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(waypointBtn, waypoint, LV_EVENT_PRESSED, NULL);

    // Track Button
    lv_obj_t *trackBtn = lv_img_create(buttonBar);
    lv_img_set_src(trackBtn, "F:/track.bin");
    lv_obj_add_flag(trackBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(trackBtn, track, LV_EVENT_PRESSED, NULL);

    // Settings Button
    lv_obj_t *settingsBtn = lv_img_create(buttonBar);
    lv_img_set_src(settingsBtn, "F:/settings.bin");
    lv_obj_add_flag(settingsBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(settingsBtn, settings, LV_EVENT_PRESSED, NULL);
}

/**
 * @brief Load waypoint, track options modal dialog.
 *
 */
static void load_options()
{
    lv_obj_t *option = NULL;
    if (is_waypoint)
        option = lv_msgbox_create(lv_scr_act(), "Waypoint Options", NULL, NULL, true);
    else if (is_track)
        option = lv_msgbox_create(lv_scr_act(), "Track Options", NULL, NULL, true);

    lv_obj_set_size(option, TFT_WIDTH, 128);
    lv_obj_set_pos(option, 0, TFT_HEIGHT - 200);
    lv_obj_clear_flag(option, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(((lv_msgbox_t *)option)->close_btn, close_option, LV_EVENT_PRESSED, NULL);

    // Save Button
    lv_obj_t *saveBtn = lv_img_create(option);
    lv_img_set_src(saveBtn, "F:/save.bin");
    lv_obj_add_flag(saveBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(saveBtn, save_option, LV_EVENT_PRESSED, NULL);

    // Load Button
    lv_obj_t *loadBtn = lv_img_create(option);
    lv_img_set_src(loadBtn, "F:/load.bin");
    lv_obj_add_flag(loadBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(loadBtn, load_option, LV_EVENT_PRESSED, NULL);

    // Delete Button
    lv_obj_t *deleteBtn = lv_img_create(option);
    lv_img_set_src(deleteBtn, "F:/delete.bin");
    lv_obj_add_flag(deleteBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(deleteBtn, delete_option, LV_EVENT_PRESSED, NULL);
}