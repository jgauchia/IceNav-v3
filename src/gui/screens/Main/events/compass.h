/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass screen events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Update compass heading label
 *
 * @param event
 */
static void update_heading(lv_event_t *event)
{
#ifdef ENABLE_COMPASS
    lv_obj_t *compass = lv_event_get_target(event);
    lv_label_set_text_fmt(compass, "%5d\xC2\xB0", heading);
    lv_img_set_angle(compass_img, -(heading * 10));
#endif
}

/**
 * @brief Update latitude label
 *
 * @param event
 */
static void update_latitude(lv_event_t *event)
{
    lv_obj_t *lat = lv_event_get_target(event);
    lv_label_set_text(lat, Latitude_formatString(GPS.location.lat()));
}

/**
 * @brief Update longitude label
 *
 * @param event
 */
static void update_longitude(lv_event_t *event)
{
    lv_obj_t *lon = lv_event_get_target(event);
    lv_label_set_text(lon, Longitude_formatString(GPS.location.lng()));
}

/**
 * @brief Upate altitude label
 *
 * @param event
 */
static void update_altitude(lv_event_t *event)
{
    lv_obj_t *alt = lv_event_get_target(event);
    lv_label_set_text_fmt(altitude, "%4d m.", (int)GPS.altitude.meters());
}

/**
 * @brief Update speed label
 *
 * @param event
 */
static void update_speed(lv_event_t *event)
{
    lv_obj_t *speed = lv_event_get_target(event);
    lv_label_set_text_fmt(speed, "%3d Km/h", (int)GPS.speed.kmph());
}