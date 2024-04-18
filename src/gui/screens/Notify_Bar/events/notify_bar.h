/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief Notify Bar Events
 * @version 0.1.8
 * @date 2024-04
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
static void updateBatt(lv_event_t *event)
{
    if (battLevel <= 160 && battLevel > 140)
        lv_label_set_text_static(battery, "  " LV_SYMBOL_CHARGE);
    else if (battLevel <= 140 && battLevel > 80)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_FULL);
    else if (battLevel <= 80 && battLevel > 60)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_3);
    else if (battLevel <= 60 && battLevel > 40)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_2);
    else if (battLevel <= 40 && battLevel > 20)
        lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_1);
    else if (battLevel <= 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
}

/**
 * @brief GPS Fix Mode update event
 *
 */
static void updateFixMode(lv_event_t *event)
{
    lv_obj_t *mode = lv_event_get_target_obj(event);
    if (fixMode.isValid() && fix_old != atoi(fixMode.value()))
    {
        switch (atoi(fixMode.value()))
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
        fix_old = atoi(fixMode.value());
    }
}

/**
 * @brief Time update event
 *
 */
static void updateTime(lv_event_t *event)
{
    lv_obj_t *time = lv_event_get_target_obj(event);
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
static void updateGpsCount(lv_event_t *event)
{
    lv_obj_t *gps_num = lv_event_get_target_obj(event);
    if (GPS.satellites.isValid())
        lv_label_set_text_fmt(gps_num, LV_SYMBOL_GPS "%2d", GPS.satellites.value());
    else
        lv_label_set_text_fmt(gps_num, LV_SYMBOL_GPS "%2d", 0);
}

/**
 * @brief Update notify bar info timer
 *
 */
void updateNotifyBar(lv_timer_t *t)
{
    lv_obj_send_event(gpsTime, LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_send_event(gpsCount, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_send_event(gpsFixMode, LV_EVENT_VALUE_CHANGED, NULL);

    switch (atoi(fix.value()))
    {
    case 0:
        lv_led_off(gpsFix);
        break;
    case 1:
        lv_led_toggle(gpsFix);
        break;
    case 2:
        lv_led_toggle(gpsFix);
        break;
    default:
        lv_led_off(gpsFix);
        break;
    }

    battLevel = batteryRead();
    if (battLevel != battLevelOld)
    {
        lv_obj_send_event(battery, LV_EVENT_VALUE_CHANGED, NULL);
        battLevelOld = battLevel;
    }

#ifdef ENABLE_BME
    if ((uint8_t)(bme.readTemperature()) != tempOld)
    {
        lv_label_set_text_fmt(temp, "%02d\xC2\xB0", (uint8_t)(bme.readTemperature()));
        tempOld = (uint8_t)(bme.readTemperature());
    }
#endif
}