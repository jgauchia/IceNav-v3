/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Notify Bar Events
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief  Notify Bar update time
 *
 */
#define UPDATE_NOTIFY_PERIOD 1000

/**
 * @brief Battery update event
 *
 */
static void update_batt(lv_event_t *event)
{
    if (batt_level <= 160 && batt_level > 140)
        lv_label_set_text_static(battery, "  " LV_SYMBOL_CHARGE);
    else if (batt_level <= 140 && batt_level > 80)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_FULL);
    else if (batt_level <= 80 && batt_level > 60)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_3);
    else if (batt_level <= 60 && batt_level > 40)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_2);
    else if (batt_level <= 40 && batt_level > 20)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_1);
    else if (batt_level <= 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
}

/**
 * @brief GPS Fix Mode update event
 *
 */
static void update_fix_mode(lv_event_t *event)
{
    lv_obj_t *mode = lv_event_get_target(event);
    if (fix_mode.isValid() && fix_old != atoi(fix_mode.value()))
    {
        switch (atoi(fix_mode.value()))
        {
        case 1:
            lv_label_set_text_static(mode, "--");
            break;
        case 2:
            lv_label_set_text_static(mode, "2D");
            break;
        case 3:
            lv_label_set_text_static(mode, "3D");
            break;
        default:
            lv_label_set_text_static(mode, "--");
            break;
        }
        fix_old = atoi(fix_mode.value());
    }
}

/**
 * @brief Time update event
 *
 */
static void update_time(lv_event_t *event)
{
    lv_obj_t *time = lv_event_get_target(event);
    // UTC Time
    utc = now();
    // Local Time
    local = CE.toLocal(utc);
    lv_label_set_text_fmt(time, "%02d:%02d:%02d", hour(local), minute(local), second(local));
}

/**
 * @brief Update satellite count event
 *
 */
static void update_gps_count(lv_event_t *event)
{
    lv_obj_t *gps_num = lv_event_get_target(event);
    if (GPS.satellites.isValid())
        lv_label_set_text_fmt(gps_num, LV_SYMBOL_GPS "%2d", GPS.satellites.value());
    else
        lv_label_set_text_fmt(gps_num, LV_SYMBOL_GPS "%2d", 0);
}

/**
 * @brief Update notify bar info timer
 *
 */
void update_notify_bar(lv_timer_t *t)
{
    lv_event_send(gps_time, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_send(gps_count, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_send(gps_fix_mode, LV_EVENT_VALUE_CHANGED, NULL);

    batt_level = battery_read();
    if (batt_level != batt_level_old)
    {
        lv_event_send(battery, LV_EVENT_VALUE_CHANGED, NULL);
        batt_level_old = batt_level;
    }

#ifdef ENABLE_BME
    if ((uint8_t)(bme.readTemperature()) != temp_old)
    {
        lv_label_set_text_fmt(temp, "%02d\xC2\xB0", (uint8_t)(bme.readTemperature()));
        temp_old = (uint8_t)(bme.readTemperature());
    }
#endif
}