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
    notifyBar = lv_obj_create(mainScreen);
    lv_obj_set_size(notifyBar, TFT_WIDTH, 22);
    lv_obj_set_pos(notifyBar, 0, 0);
    lv_obj_set_flex_flow(notifyBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(notifyBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(notifyBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t style_bar;
    lv_style_init(&style_bar);
    lv_style_set_bg_opa(&style_bar, LV_OPA_0);
    lv_style_set_border_opa(&style_bar, LV_OPA_0);
    lv_obj_add_style(notifyBar, &style_bar, LV_PART_MAIN);

    gps_time = lv_label_create(notifyBar);
    lv_obj_set_width(gps_time, 145);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(local), minute(local), second(local));
    lv_obj_add_event_cb(gps_time, update_time, LV_EVENT_VALUE_CHANGED, NULL);

    temp = lv_label_create(notifyBar);
    lv_label_set_text_static(temp, "--\xC2\xB0");

    sdcard = lv_label_create(notifyBar);
    if (sdloaded)
        lv_label_set_text_static(sdcard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text_static(sdcard, " ");

    gps_count = lv_label_create(notifyBar);
    lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", 0);
    lv_obj_add_event_cb(gps_count, update_gps_count, LV_EVENT_VALUE_CHANGED, NULL);

    gps_fix = lv_led_create(notifyBar);
    lv_led_set_color(gps_fix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gps_fix, 7, 7);
    lv_led_off(gps_fix);

    gps_fix_mode = lv_label_create(notifyBar);
    lv_obj_set_style_text_font(gps_fix_mode, &lv_font_montserrat_10, 0);
    lv_label_set_text_static(gps_fix_mode, "--");
    lv_obj_add_event_cb(gps_fix_mode, update_fix_mode, LV_EVENT_VALUE_CHANGED, NULL);

    battery = lv_label_create(notifyBar);
    lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_event_cb(battery, update_batt, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_t *timer_notify_bar = lv_timer_create(update_notify_bar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timer_notify_bar);
}
