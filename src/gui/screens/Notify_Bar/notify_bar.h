/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Notify Bar
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Notify Bar Screen
 *
 */
static lv_obj_t *notifyBar;

/**
 * @brief Notify Bar screen objects
 *
 */
static lv_obj_t *gps_time;
static lv_obj_t *gps_count;
static lv_obj_t *gps_fix;
static lv_obj_t *gps_fix_mode;
static lv_obj_t *battery;
static lv_obj_t *sdcard;
static lv_obj_t *temp;

/**
 * @brief Notify Bar events include
 *
 */
#include "gui/screens/Notify_Bar/events/notify_bar.h"

/**
 * @brief Create a notify bar
 *
 */
void create_notify_bar()
{
    battery = lv_label_create(lv_scr_act());
    lv_obj_set_size(battery, 20, 10);
    lv_obj_set_pos(battery, TFT_WIDTH - 25, 2);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_event_cb(battery, update_batt, LV_EVENT_VALUE_CHANGED, NULL);

    gps_fix_mode = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_fix_mode, 40, 10);
    lv_obj_set_pos(gps_fix_mode, TFT_WIDTH - 45, 5);
    lv_obj_set_style_text_font(gps_fix_mode, &lv_font_montserrat_10, 0);
    lv_label_set_text(gps_fix_mode, "--");
    lv_obj_add_event_cb(gps_fix_mode, update_fix_mode, LV_EVENT_VALUE_CHANGED, NULL);

    gps_fix = lv_led_create(lv_scr_act());
    lv_led_set_color(gps_fix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gps_fix, 7, 7);
    lv_obj_set_pos(gps_fix, TFT_WIDTH - 60, 5);
    lv_led_off(gps_fix);

    gps_count = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_count, 50, 10);
    lv_obj_set_pos(gps_count, TFT_WIDTH - 98, 2);
    lv_obj_add_event_cb(gps_count, update_gps_count, LV_EVENT_VALUE_CHANGED, NULL);

    sdcard = lv_label_create(lv_scr_act());
    lv_obj_set_size(sdcard, 20, 15);
    lv_obj_set_pos(sdcard, TFT_WIDTH - 115, 2);
    if (sdloaded)
        lv_label_set_text(sdcard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text(sdcard, " ");

#ifdef ENABLE_BME
    temp = lv_label_create(lv_scr_act());
    lv_obj_set_size(temp, 50, 10);
    lv_obj_set_pos(temp, TFT_WIDTH - 145, 2);
    lv_label_set_text(temp, "--\xC2\xB0");
#endif

    gps_time = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_time, 100, 15);
    lv_obj_set_pos(gps_time, 0, 0);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(local), minute(local), second(local));
    lv_obj_add_event_cb(gps_time, update_time, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_t *timer_notify_bar = lv_timer_create(update_notify_bar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timer_notify_bar);
}