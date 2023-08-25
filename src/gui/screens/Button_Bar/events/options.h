/**
 * @file options.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Waypoint, track events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Options box close event
 *
 * @param event
 */
static void close_option(lv_event_t *event)
{
    is_main_screen = true;
    is_option_loaded = false;
}

/**
 * @brief Save Option
 *
 * @param event
 */
static void save_option(lv_event_t *event)
{
    log_v("Save Option");
    is_main_screen = true;
    is_option_loaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Load Option
 *
 * @param event
 */
static void load_option(lv_event_t *event)
{
    log_v("Load Option");
    is_main_screen = true;
    is_option_loaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Delete Option
 *
 * @param event
 */
static void delete_option(lv_event_t *event)
{
    log_v("Delete Option");
    is_main_screen = true;
    is_option_loaded = false;
    lv_msgbox_close(option);
}