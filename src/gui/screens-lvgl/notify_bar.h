/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Notify Bar
 * @version 0.1
 * @date 2022-10-16
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
#include "gui/events/notify_bar.h"

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

    gps_fix_mode = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_fix_mode, 50, 20);
    lv_obj_set_pos(gps_fix_mode, TFT_WIDTH - 45, 5);
    lv_obj_set_style_text_font(gps_fix_mode, &lv_font_montserrat_10, 0);

    gps_fix = lv_led_create(lv_scr_act());
    lv_led_set_color(gps_fix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gps_fix, 10, 10);
    lv_obj_set_pos(gps_fix, TFT_WIDTH - 60, 5);
    lv_led_off(gps_fix);

    gps_count = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_count, 50, 20);
    lv_obj_set_pos(gps_count, TFT_WIDTH - 98, 2);

    sdcard = lv_label_create(lv_scr_act());
    lv_obj_set_size(sdcard, 20, 20);
    lv_obj_set_pos(sdcard, TFT_WIDTH - 115, 2);

#if ENABLE_BME
    temp = lv_label_create(lv_scr_act());
    lv_obj_set_size(temp, 50, 20);
    lv_obj_set_pos(temp, TFT_WIDTH - 145, 2);
#endif

    gps_time = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_time, 100, 20);
    lv_obj_set_pos(gps_time, 0, 0);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);

    lv_timer_t *timer_notify_bar = lv_timer_create(update_notify_bar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timer_notify_bar);
}
