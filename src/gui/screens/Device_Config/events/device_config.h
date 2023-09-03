/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Device Config events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Back button event
 *
 * @param event
 */
static void device_conf_back(lv_event_t *event)
{
    lv_scr_load(settingsScreen);
}

/**
 * @brief Configure Map rotation event
 *
 * @param event
 */
static void configure_map_rotation(lv_event_t *event)
{
    map_rotation = lv_obj_has_state(map_switch, LV_STATE_CHECKED);
    preferences.begin("ICENAV",false);
    preferences.putBool("Map_rot",map_rotation);
    preferences.end();
}