/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Notify Bar
 * @version 0.1.8
 * @date 2024-04
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
static lv_obj_t *gpsTime;
static lv_obj_t *gpsCount;
static lv_obj_t *gpsFix;
static lv_obj_t *gpsFixMode;
static lv_obj_t *battery;
static lv_obj_t *sdCard;
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
void createNotifyBar()
{
    notifyBar = lv_obj_create(mainScreen);
    lv_obj_set_size(notifyBar, TFT_WIDTH, 22);
    lv_obj_set_pos(notifyBar, 0, 0);
    lv_obj_set_flex_flow(notifyBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(notifyBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(notifyBar, LV_OBJ_FLAG_SCROLLABLE);
    static lv_style_t styleBar;
    lv_style_init(&styleBar);
    lv_style_set_bg_opa(&styleBar, LV_OPA_0);
    lv_style_set_border_opa(&styleBar, LV_OPA_0);
    lv_obj_add_style(notifyBar, &styleBar, LV_PART_MAIN);

    lv_obj_t *label;
    
    gpsTime = lv_label_create(notifyBar);
    lv_obj_set_width(gpsTime, 140);
    lv_obj_set_style_text_font(gpsTime, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(gpsTime, "%02d:%02d:%02d", hour(local), minute(local), second(local));
    lv_obj_add_event_cb(gpsTime, updateTime, LV_EVENT_VALUE_CHANGED, NULL);

    temp = lv_label_create(notifyBar);
    lv_label_set_text_static(temp, "--\xC2\xB0");

    sdCard = lv_label_create(notifyBar);
    if (sdloaded)
        lv_label_set_text_static(sdCard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text_static(sdCard, " ");

    gpsCount = lv_label_create(notifyBar);
    lv_label_set_text_fmt(gpsCount, LV_SYMBOL_GPS "%2d", 0);
    lv_obj_add_event_cb(gpsCount, updateGpsCount, LV_EVENT_VALUE_CHANGED, NULL);

    gpsFix = lv_led_create(notifyBar);
    lv_led_set_color(gpsFix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gpsFix, 7, 7);
    lv_led_off(gpsFix);

    gpsFixMode = lv_label_create(notifyBar);
    lv_obj_set_style_text_font(gpsFixMode, &lv_font_montserrat_10, 0);
    lv_label_set_text_static(gpsFixMode, "--");
    lv_obj_add_event_cb(gpsFixMode, updateFixMode, LV_EVENT_VALUE_CHANGED, NULL);

    battery = lv_label_create(notifyBar);
    lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_event_cb(battery, updateBatt, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_t *timerNotifyBar = lv_timer_create(updateNotifyBar, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timerNotifyBar);
}
