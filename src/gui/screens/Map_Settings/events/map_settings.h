/**
 * @file map_settings.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Map Settings events
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief Back button event
 *
 * @param event
 */
static void mapSettingsBack(lv_event_t *event)
{
    lv_screen_load(settingsScreen);
}

/**
 * @brief Configure Map type event
 *
 * @param event
 */
static void configureMapType(lv_event_t *event)
{
    isVectorMap = lv_obj_has_state(mapType, LV_STATE_CHECKED);
    saveMapType(isVectorMap);
    mapSprite.deleteSprite();
    mapRotSprite.deleteSprite();
    if (!isVectorMap)
        mapSprite.createSprite(768, 768);

    if (isVectorMap)
    {
        MIN_ZOOM = 1;
        MAX_ZOOM = 4;
    }
    else
    {
        MIN_ZOOM = 6;
        MAX_ZOOM = 17;
    }
    lv_spinbox_set_range(zoomLevel, MIN_ZOOM, MAX_ZOOM);
}

/**
 * @brief Configure Map rotation event
 *
 * @param event
 */
static void configureMapRotation(lv_event_t *event)
{
    isMapRotation = lv_obj_has_state(mapSwitch, LV_STATE_CHECKED);
    saveMapRotation(isMapRotation);
}

/**
 * @brief Increment default zoom value event
 *
 * @param event
 */
static void incrementZoom(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_increment(zoomLevel);
        defaultZoom = (uint8_t)lv_spinbox_get_value(zoomLevel);
        saveDefaultZoom(defaultZoom);
    }
}

/**
 * @brief Decrement default zoom value event
 *
 */
static void decrementZoom(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_decrement(zoomLevel);
        defaultZoom = (uint8_t)lv_spinbox_get_value(zoomLevel);
        saveDefaultZoom(defaultZoom);
    }
}

/**
 * @brief Show Compass option event
 *
 * @param event
 */
static void showCompass(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    showMapCompass = lv_obj_has_state(obj, LV_STATE_CHECKED);
    saveShowCompass(showMapCompass);
}

/**
 * @brief Show Speed option event
 *
 * @param event
 */
static void showSpeed(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    showMapSpeed = lv_obj_has_state(obj, LV_STATE_CHECKED);
    saveShowSpeed(showMapSpeed);
}

/**
 * @brief Show Map Scale option event
 *
 * @param event
 */
static void showScale(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target_obj(event);
    showMapScale = lv_obj_has_state(obj, LV_STATE_CHECKED);
    saveShowScale(showMapScale);
}