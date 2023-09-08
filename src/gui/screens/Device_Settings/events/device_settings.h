/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Device Settings events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Back button event
 *
 * @param event
 */
static void device_settings_back(lv_event_t *event)
{
    lv_scr_load(settingsScreen);
}