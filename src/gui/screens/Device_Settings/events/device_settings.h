/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Device Settings events
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief Back button event
 *
 * @param event
 */
static void deviceSettingsBack(lv_event_t *event)
{
    lv_screen_load(settingsScreen);
}

/**
 * @brief GPS Speed event
 * 
 * @param event 
 */
static void setGpsSpeed(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t*)lv_event_get_target(event);
    gpsBaud = lv_dropdown_get_selected(obj);
    saveGPSBaud(gpsBaud);
}

/**
 * @brief GPS Update Rate event
 * 
 * @param event 
 */
static void setGpsUpdateRate(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t*)lv_event_get_target(event);
    gpsUpdate = lv_dropdown_get_selected(obj);
    saveGPSUpdateRate(gpsUpdate);
}