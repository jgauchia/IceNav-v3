/**
 * @file searchSatScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.2.4
 * @date 2025-12
 */

#include "searchSatScr.hpp"
#include "esp_timer.h"

/**
 * @brief Get system uptime in milliseconds using ESP-IDF timer.
 * @return uint32_t Milliseconds since boot.
 */
static inline uint32_t millis_idf() { return (uint32_t)(esp_timer_get_time() / 1000); }

static unsigned long millisActual = 0;        /**< Stores the current timestamp in milliseconds */
static bool skipSearch = false;               /**< Flag to indicate if satellite search should be skipped */
bool isSearchingSat = true;                   /**< Flag to indicate if satellite search is in progress */
extern uint8_t activeTile;                    /**< Index of the currently active tile */
extern lv_timer_t *mainTimer;                 /**< Main Screen Timer */
lv_timer_t *searchTimer;                      /**< Timer for satellite search process */

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
    lv_timer_resume(mainTimer);
}

/**
 * @brief Search valid GPS signal
 *
 * @details Checks for a valid GPS fix or a skip command.
 *
 * @param searchTimer LVGL timer pointer associated with the satellite search.
 */
void searchGPS(lv_timer_t *searchTimer)
{
    static uint8_t fixConfirmCount = 0;  // Confirm fix is stable

    if (isGpsFixed)
    {
        fixConfirmCount++;
        // Wait for 5 consecutive checks (~500ms) to confirm stable fix
        if (fixConfirmCount >= 5)
        {
            fixConfirmCount = 0;
            lv_timer_del(searchTimer);
            lv_timer_resume(mainTimer);
            isSearchingSat = false;
            loadMainScreen();
        }
        return;
    }
    else
    {
        fixConfirmCount = 0;  // Reset if fix lost
    }

    if (skipSearch)
    {
        skipSearch = false;  // Reset flag
        fixConfirmCount = 0;
        lv_timer_del(searchTimer);
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
    searchTimer = lv_timer_create(searchGPS, 100, NULL);
    lv_timer_pause(searchTimer);
    lv_timer_pause(mainTimer);

    searchSatScreen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(searchSatScreen);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_label_set_text(label, textSearch);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -100);

    lv_obj_t *spinner = lv_spinner_create(searchSatScreen);
    lv_obj_set_size(spinner, 130, 130);
    lv_spinner_set_anim_params(spinner, 2000, 200);
    lv_obj_center(spinner);

    lv_obj_t *satImg = lv_img_create(searchSatScreen);
    lv_img_set_src(satImg, satIconFile);
    lv_obj_set_align(satImg, LV_ALIGN_CENTER);

    // Button Bar
    lv_obj_t *buttonBar = lv_obj_create(searchSatScreen);
    lv_obj_set_size(buttonBar, TFT_WIDTH, 68 * scaleBut);
    lv_obj_set_pos(buttonBar, 0, TFT_HEIGHT - 80 * scaleBut);
    lv_obj_set_flex_flow(buttonBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonBar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t styleBar;
    lv_style_init(&styleBar);
    lv_style_set_bg_opa(&styleBar, LV_OPA_0);
    lv_style_set_border_opa(&styleBar, LV_OPA_0);
    lv_obj_add_style(buttonBar, &styleBar, LV_PART_MAIN);

    lv_obj_t *imgBtn;
    
    // Settings Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, confIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_zoom(imgBtn,buttonScale);
    lv_obj_update_layout(imgBtn);
    lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
    lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"settings");
    
    // Skip Button
    imgBtn = lv_img_create(buttonBar);
    lv_img_set_src(imgBtn, skipIconFile);
    lv_obj_add_flag(imgBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_zoom(imgBtn,buttonScale);
    lv_obj_update_layout(imgBtn);
    lv_obj_set_style_size(imgBtn,48 * scaleBut, 48 * scaleBut, 0);
    lv_obj_add_event_cb(imgBtn, buttonEvent, LV_EVENT_PRESSED, (char*)"skip");
}
