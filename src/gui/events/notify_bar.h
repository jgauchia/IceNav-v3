/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Notify Bar Events
 * @version 0.1
 * @date 2023-02-22
 */

/**
 * @brief  Notify Bar update time
 *
 */
#define UPDATE_NOTIFY_PERIOD 1000

/**
 * @brief Update notify bar info event
 *
 */
void update_notify_bar(lv_timer_t *t)
{
    // UTC Time
    utc = now();
    // Local Time
    local = CE.toLocal(utc);
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(local), minute(local), second(local));

    lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", GPS.satellites.value());

    switch (atoi(fix_mode.value()))
    {
    case 1:
        lv_label_set_text(gps_fix_mode, "--");
        lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", 0);
        break;
    case 2:
        lv_label_set_text(gps_fix_mode, "2D");
        break;
    case 3:
        lv_label_set_text(gps_fix_mode, "3D");
        break;
    default:
        lv_label_set_text(gps_fix_mode, "--");
        lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", 0);
        break;
    }

    switch (atoi(fix.value()))
    {
    case 0:
        lv_led_off(gps_fix);
        lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", 0);
        break;
    case 1:
        lv_led_toggle(gps_fix);
        break;
    case 2:
        lv_led_toggle(gps_fix);
        break;
    default:
        lv_led_off(gps_fix);
        lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", 0);
        break;
    }

    if (sdloaded)
        lv_label_set_text(sdcard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text(sdcard, " ");

    batt_level = battery_read();

    if (batt_level <= 150 && batt_level > 140)
        lv_label_set_text(battery, "  " LV_SYMBOL_CHARGE);
    else if (batt_level <= 140 && batt_level > 80)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_FULL);
    else if (batt_level <= 80 && batt_level > 60)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_3);
    else if (batt_level <= 60 && batt_level > 40)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_2);
    else if (batt_level <= 40 && batt_level > 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_1);
    else if (batt_level <= 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);

#ifdef ENABLE_BME
    lv_label_set_text_fmt(temp, "%02d\xC2\xB0", int(bme.readTemperature()));
#endif
}