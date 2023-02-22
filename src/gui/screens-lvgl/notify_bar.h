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

    gps_time = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_time, 100, 20);
    lv_obj_set_pos(gps_time, 0, 0);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);
    
    gps_count = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_count, 50, 20);
    lv_obj_set_pos(gps_count, TFT_WIDTH - 58, 2);
    lv_label_set_recolor(gps_count, true);
    
    sdcard = lv_label_create(lv_scr_act());
    lv_obj_set_size(sdcard, 20, 20);
    lv_obj_set_pos(sdcard, TFT_WIDTH - 75, 2);
    
#if ENABLE_BME
    temp = lv_label_create(lv_scr_act());
    lv_obj_set_size(temp, 50, 20);
    lv_obj_set_pos(temp, TFT_WIDTH - 105, 2);
#endif

    lv_timer_t *timer_notify_bar = lv_timer_create(update_notify_bar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timer_notify_bar);
}


