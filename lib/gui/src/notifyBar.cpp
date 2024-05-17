/**
 * @file notifyBar.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief LVGL - Notify Bar Screen
 * @version 0.1.8
 * @date 2024-05
 */

#include "notifyBar.hpp"

lv_obj_t *mainScreen;
lv_obj_t *notifyBar;

/**
 * @brief Update notify bar event
 *
 * @param event
 */
void updateNotifyBar(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);

    if (obj == gpsTime)
        lv_label_set_text_fmt(obj, timeFormat, hour(now()), minute(now()), second(now()));
    if (obj == temp)
        lv_label_set_text_fmt(obj, "%02d\xC2\xB0", tempValue);
    if (obj == gpsCount)
    {
        if (GPS.satellites.isValid())
            lv_label_set_text_fmt(obj, LV_SYMBOL_GPS "%2d", GPS.satellites.value());
        else
            lv_label_set_text_fmt(obj, LV_SYMBOL_GPS "%2d", 0);
    }
    if (obj == battery)
    {
        if (battLevel <= 160 && battLevel > 140)
            lv_label_set_text_static(obj, "  " LV_SYMBOL_CHARGE);
        else if (battLevel <= 140 && battLevel > 80)
            lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_FULL);
        else if (battLevel <= 80 && battLevel > 60)
            lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_3);
        else if (battLevel <= 60 && battLevel > 40)
            lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_2);
        else if (battLevel <= 40 && battLevel > 20)
            lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_1);
        else if (battLevel <= 20)
            lv_label_set_text(obj, LV_SYMBOL_BATTERY_EMPTY);
    }
    if (obj == gpsFixMode)
    {
        if (fixMode.isValid() && fix_old != atoi(fixMode.value()))
        {
            switch (atoi(fixMode.value()))
            {
            case 1:
                lv_label_set_text_static(obj, "--");
                break;
            case 2:
                lv_label_set_text_static(obj, "2D");
                break;
            case 3:
                lv_label_set_text_static(obj, "3D");
                break;
            default:
                lv_label_set_text_static(obj, "--");
                break;
            }
            fix_old = atoi(fixMode.value());
        }
    }
}

/**
 * @brief Update notify bar info timer
 *
 */
void updateNotifyBarTimer(lv_timer_t *t)
{
    lv_obj_send_event(gpsTime, LV_EVENT_VALUE_CHANGED, NULL);
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

    if (tempValue != tempOld)
    {
        lv_obj_send_event(temp, LV_EVENT_VALUE_CHANGED, NULL);
        tempOld = tempValue;
    }
    
    if (battLevel != battLevelOld)
    {
        lv_obj_send_event(battery, LV_EVENT_VALUE_CHANGED, NULL);
        battLevelOld = battLevel;
    }
}

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
    lv_label_set_text_fmt(gpsTime, timeFormat, hour(local), minute(local), second(local));
    lv_obj_add_event_cb(gpsTime, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

    temp = lv_label_create(notifyBar);
    lv_label_set_text_static(temp, "--\xC2\xB0");
    lv_obj_add_event_cb(temp, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

    sdCard = lv_label_create(notifyBar);
    if (isSdLoaded)
        lv_label_set_text_static(sdCard, LV_SYMBOL_SD_CARD);
    else
        lv_label_set_text_static(sdCard, " ");

    gpsCount = lv_label_create(notifyBar);
    lv_label_set_text_fmt(gpsCount, LV_SYMBOL_GPS "%2d", 0);
    lv_obj_add_event_cb(gpsCount, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

    gpsFix = lv_led_create(notifyBar);
    lv_led_set_color(gpsFix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gpsFix, 7, 7);
    lv_led_off(gpsFix);

    gpsFixMode = lv_label_create(notifyBar);
    lv_obj_set_style_text_font(gpsFixMode, &lv_font_montserrat_10, 0);
    lv_label_set_text_static(gpsFixMode, "--");
    lv_obj_add_event_cb(gpsFixMode, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

    battery = lv_label_create(notifyBar);
    lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_add_event_cb(battery, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_t *timerNotifyBar = lv_timer_create(updateNotifyBarTimer, UPDATE_NOTIFY_PERIOD, NULL);
    lv_timer_ready(timerNotifyBar);
}
