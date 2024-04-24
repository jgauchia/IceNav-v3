/**
 * @file buttonBar.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Button Bar 
 * @version 0.1.8
 * @date 2024-04
 */

#include "buttonBar.hpp"

bool isWaypointOpt = false;
bool isTrackOpt = false;
bool isOptionLoaded = false;

lv_obj_t *settingsScreen;


/**
 * @brief Settings Button event
 *
 * @param event
 */
void settings(lv_event_t *event)
{
    log_v("Settings");
    isMainScreen = false;
    lv_screen_load(settingsScreen);
}

/**
 * @brief Waypoint Button event
 *
 * @param event
 */
void waypoint(lv_event_t *event)
{
    log_v("Waypoint");
    isMainScreen = false;
    isWaypointOpt = true;
    isTrackOpt = false;
    if (!isOptionLoaded)
    {
        isOptionLoaded = true;
        //loadOptions();
    }
}

/**
 * @brief Track Button event
 *
 * @param event
 */
void track(lv_event_t *event)
{
    log_v("Track");
    isMainScreen = false;
    isTrackOpt = true;
    isWaypointOpt = false;
    if (!isOptionLoaded)
    {
        isOptionLoaded = true;
        //loadOptions();
    }
}

/**
 * @brief Save Option
 *
 * @param event
 */
void saveOption(lv_event_t *event)
{
    log_v("Save Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Load Option
 *
 * @param event
 */
void loadOption(lv_event_t *event)
{
    log_v("Load Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Delete Option
 *
 * @param event
 */
void deleteOption(lv_event_t *event)
{
    log_v("Delete Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Create button bar screen
 *
 */
void createButtonBarScr()
{
    // Button Bar
    buttonBar = lv_obj_create(mainScreen);
    lv_obj_set_size(buttonBar, TFT_WIDTH, 68);
    lv_obj_set_pos(buttonBar, 0, TFT_HEIGHT - 80);
    lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t styleBar;
    lv_style_init(&styleBar);
    lv_style_set_bg_opa(&styleBar, LV_OPA_0);
    lv_style_set_border_opa(&styleBar, LV_OPA_0);
    lv_obj_add_style(buttonBar, &styleBar, LV_PART_MAIN);

    lv_obj_t *imgBtn;

    // Waypoint Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, waypointIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, waypoint, LV_EVENT_PRESSED, NULL);

    // Track Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, trackIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, track, LV_EVENT_PRESSED, NULL);

    // Settings Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, settingsIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, settings, LV_EVENT_PRESSED, NULL);
}

/**
 * @brief Load waypoint, track options modal dialog.
 *
 */
void loadOptions()
{
    option = lv_msgbox_create(lv_scr_act());
    if (isWaypointOpt)
    {
        // lv_msgbox_add_title(option,"Waypoint Options");
        //  option = lv_msgbox_create(lv_scr_act(), "Waypoint Options", NULL, NULL, true);
    }
    else if (isTrackOpt)
    {
        // lv_msgbox_add_title(option,"Track Options");
        // option = lv_msgbox_create(lv_scr_act(), "Track Options", NULL, NULL, true);
    }
    lv_msgbox_add_close_button(option);

    lv_obj_set_size(option, TFT_WIDTH, 128);
    lv_obj_set_pos(option, 0, TFT_HEIGHT - 200);
    lv_obj_clear_flag(option, LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_add_event_cb(lv_msgbox_get_close_btn(option), close_option, LV_EVENT_PRESSED, NULL);

    lv_obj_t *imgBtn;

    // Save Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, saveIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, saveOption, LV_EVENT_PRESSED, NULL);

    // Load Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, loadIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, loadOption, LV_EVENT_PRESSED, NULL);

    // Delete Button
    imgBtn = lv_img_create(option);
    lv_img_set_src(imgBtn, deleteIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(imgBtn, deleteOption, LV_EVENT_PRESSED, NULL);
}
