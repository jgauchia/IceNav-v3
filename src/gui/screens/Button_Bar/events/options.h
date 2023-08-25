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
