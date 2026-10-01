/**
 * @file searchSatScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.3.0
 * @date 2026-10
 */

#include "searchSatScr.hpp"
#include "esp_timer.h"

static bool skipSearch = false;               /**< Flag to indicate if satellite search should be skipped */
bool isSearchingSat = true;                   /**< Flag to indicate if satellite search is in progress */
extern uint8_t activeTile;                    /**< Index of the currently active tile */
static lv_timer_t *searchTimer = NULL;        /**< Timer for satellite search process */

/**
 * @brief Button events
 *
 * @details Handles button events for the search screen. 
 *
 * @param event LVGL event pointer.
 */
void buttonEvent(lv_event_t *event)
{
    char *option = (char *)lv_event_get_user_data(event);
    if (strcmp(option,"skip") == 0)
        skipSearch = true;
    if (strcmp(option,"settings") == 0)
        lv_screen_load(settingsScreen);
}
/**
 * @brief Delete the satellite search timer
 *
 * @details Deletes the given timer and clears the module handle when both point to the same timer, so
 *          no later caller can resume or delete a freed timer. Safe to call with NULL because the LVGL
 *          timer API is built with LV_USE_ASSERT_NULL disabled.
 *
 * @param timer LVGL timer pointer to delete.
 */
static void deleteSearchTimer(lv_timer_t *timer)
{
    if (timer == NULL)
        return;
    lv_timer_delete(timer);
    if (searchTimer == timer)
        searchTimer = NULL;
}

/**
 * @brief Search valid GPS signal
 *
 * @details Checks for a valid GPS fix or a skip command.
 *
 * @param timer LVGL timer pointer associated with the satellite search.
 */
void searchGPS(lv_timer_t *timer)
{
    static uint8_t fixConfirmCount = 0;  // Confirm fix is stable

    if (isGpsFixed)
    {
        fixConfirmCount++;
        // Wait for 5 consecutive checks (~500ms) to confirm stable fix
        if (fixConfirmCount >= 5)
        {
            fixConfirmCount = 0;
            deleteSearchTimer(timer);
            isSearchingSat = false;
            loadMainScreen();
        }
        return;
    }
    else
        fixConfirmCount = 0;  // Reset if fix lost

    if (skipSearch)
    {
        skipSearch = false;  // Reset flag
        fixConfirmCount = 0;
        deleteSearchTimer(timer);
        isSearchingSat = false;
        zoom = defaultZoom;
        activeTile = 3;
        lv_tileview_set_tile_by_index(tilesScreen, 3, 0, LV_ANIM_OFF);
        loadMainScreen();
    }
}

/**
 * @brief Create Satellite Search Screen
 *
 * @details Creates the satellite search screen 
 */
void createSearchSatScr()
{
    searchSatScreen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(searchSatScreen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text(label, textSearch);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -100 * scale);

    lv_obj_t *spinner = lv_spinner_create(searchSatScreen);
    lv_obj_set_size(spinner, 130 * scale, 130 * scale);
    lv_spinner_set_anim_params(spinner, 2000, 200);
    lv_obj_center(spinner);

    lv_obj_t *satImg = lv_image_create(searchSatScreen);
    lv_image_set_src(satImg, satIconFile);
    lv_image_set_scale(satImg, iconScale);
    lv_obj_set_align(satImg, LV_ALIGN_CENTER);

    // Button Bar
    lv_obj_t *buttonBar = lv_obj_create(searchSatScreen);
    lv_obj_set_size(buttonBar, TFT_WIDTH, 68 * scaleBut);
    lv_obj_set_pos(buttonBar, 0, TFT_HEIGHT - 80 * scaleBut);
    lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(buttonBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t styleBar;
    lv_style_init(&styleBar);
    lv_style_set_bg_opa(&styleBar, LV_OPA_0);
    lv_style_set_border_opa(&styleBar, LV_OPA_0);
    lv_obj_add_style(buttonBar, &styleBar, LV_PART_MAIN);

    lv_obj_t *imgBtn;
    
    // Settings Button
    imgBtn = lv_image_create(buttonBar);
    lv_image_set_src(imgBtn, confIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_scale(imgBtn,buttonScale);
    lv_obj_update_layout(imgBtn);
    lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
    lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"settings");
    
    // Skip Button
    imgBtn = lv_image_create(buttonBar);
    lv_image_set_src(imgBtn, skipIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_scale(imgBtn,buttonScale);
    lv_obj_update_layout(imgBtn);
    lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
    lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"skip");
}

/**
 * @brief Bring the satellite search screen on screen
 *
 * @details Single entry point that owns the search timer: creates it the first time, resumes the live
 *          one on every later call, and loads the search screen. Returning from settings reuses the
 *          running timer instead of creating a second one on top of it.
 */
void resumeSatSearch()
{
    if (searchTimer == NULL)
    {
        searchTimer = lv_timer_create(searchGPS, 100, NULL);
        lv_timer_pause(searchTimer);
    }
    lv_timer_resume(searchTimer);
    lv_timer_ready(searchTimer);
    lv_screen_load(searchSatScreen);
}
