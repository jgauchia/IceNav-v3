/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Notify Bar
 * @version 0.1
 * @date 2022-10-16
 */

#define UPDATE_NOTIFY_PERIOD 1000
static lv_obj_t *notifyBar;
static lv_obj_t *gps_time;
static lv_obj_t *gps_count;
static lv_obj_t *battery;
static lv_obj_t *sdcard;
void update_notify_bar(lv_timer_t *t);

// static void drawmap(lv_event_t *event)
// {
//     if (!is_map_draw)
//     {
//         draw_png(SD, "/MAP/17/66147/48885.png",0,64);
//         is_map_draw = true;
//     }
// }

/**
 * @brief Create a notify bar
 *
 */
void create_notify_bar()
{
    battery = lv_label_create(lv_scr_act());
    lv_obj_set_size(battery, 20, 20);
    lv_obj_set_pos(battery, TFT_WIDTH - 25, 2);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);

    gps_time = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_time, 100, 20);
    lv_obj_set_pos(gps_time, 0, 0);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(), minute(), second());

    gps_count = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_count, 50, 20);
    lv_obj_set_pos(gps_count, TFT_WIDTH - 60, 2);
    lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", GPS.satellites.value());

    sdcard = lv_label_create(lv_scr_act());
    lv_obj_set_size(sdcard, 20, 20);
    lv_obj_set_pos(sdcard, TFT_WIDTH - 75, 2);
    if (sdloaded)
        lv_label_set_text(sdcard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text(sdcard, " ");

    lv_timer_t *t = lv_timer_create(update_notify_bar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(t);
}

/**
 * @brief Update notify bar info
 *
 */
void update_notify_bar(lv_timer_t *t)
{
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(), minute(), second());
    lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", GPS.satellites.value());
    if (sdloaded)
        lv_label_set_text(sdcard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text(sdcard, " ");

    batt_level = battery_read();
    if (batt_level <= 100 && batt_level > 80)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_FULL);
    else if (batt_level <= 80 && batt_level > 60)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_3);
    else if (batt_level <= 60 && batt_level > 40)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_2);
    else if (batt_level <= 40 && batt_level > 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_1);
    else if (batt_level <= 20)
        lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
}
