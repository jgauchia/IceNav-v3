/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Button Bar events
 * @version 0.1.6
 * @date 2023-06-14
 */


/**
 * @brief Settings Button event
 * 
 * @param event 
 */
static void settings(lv_event_t *event)
{
    log_v("Settings");
    is_main_screen = false;
    lv_scr_load(settingsScreen);
}

/**
 * @brief Waypoint Button event
 * 
 * @param event 
 */
static void waypoint(lv_event_t *event)
{
    log_v("Waypoint");
    is_main_screen = false;
}

/**
 * @brief Track Button event
 * 
 * @param event 
 */
static void track(lv_event_t *event)
{
    log_v("Track");
    is_main_screen = false;
}