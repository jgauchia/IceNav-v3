/**
 * @file device_config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map Settings events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Back button event
 *
 * @param event
 */
static void map_settings_back(lv_event_t *event)
{
    lv_screen_load(settingsScreen);
}

/**
 * @brief Configure Map type event
 *
 * @param event
 */
static void configure_map_type(lv_event_t *event)
{
    vector_map = lv_obj_has_state(map_type, LV_STATE_CHECKED);
    save_map_type(vector_map);
    map_spr.deleteSprite();
    map_rot.deleteSprite();
    if (!vector_map)
        map_spr.createSprite(768, 768);

    if (vector_map)
    {
        MIN_ZOOM = 1;
        MAX_ZOOM = 4;
    }
    else
    {
        MIN_ZOOM = 6;
        MAX_ZOOM = 17;
    }
    lv_spinbox_set_range(zoom_level, MIN_ZOOM, MAX_ZOOM);
}

/**
 * @brief Configure Map rotation event
 *
 * @param event
 */
static void configure_map_rotation(lv_event_t *event)
{
    map_rotation = lv_obj_has_state(map_switch, LV_STATE_CHECKED);
    save_map_rotation(map_rotation);
}

/**
 * @brief Increment default zoom value event
 *
 * @param event
 */
static void increment_zoom(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_increment(zoom_level);
        def_zoom = (uint8_t)lv_spinbox_get_value(zoom_level);
        save_default_zoom(def_zoom);
    }
}

/**
 * @brief Decrement default zoom value event
 *
 */
static void decrement_zoom(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_decrement(zoom_level);
        def_zoom = (uint8_t)lv_spinbox_get_value(zoom_level);
        save_default_zoom(def_zoom);
    }
}

/**
 * @brief Show Compass option event
 *
 * @param event
 */
static void show_compass(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    show_map_compass = lv_obj_has_state(obj, LV_STATE_CHECKED);
    save_show_compass(show_map_compass);
}

/**
 * @brief Show Speed option event
 *
 * @param event
 */
static void show_speed(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    show_map_speed = lv_obj_has_state(obj, LV_STATE_CHECKED);
    save_show_speed(show_map_speed);
}

/**
 * @brief Show Map Scale option event
 *
 * @param event
 */
static void show_scale(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    show_map_scale = lv_obj_has_state(obj, LV_STATE_CHECKED);
    save_show_scale(show_map_scale);
}