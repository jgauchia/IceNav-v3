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
 * @brief Battery update event
 *
 */
void updateBatt(lv_event_t *event)
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
void updateFixMode(lv_event_t *event)
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
void updateTime(lv_event_t *event)
{
    lv_obj_t *time = lv_event_get_target_obj(event);
    lv_label_set_text_fmt(time, timeFormat, hour(now()), minute(now()), second(now()));
}

/**
 * @brief Update satellite count event
 *
 */
void updateGpsCount(lv_event_t *event)
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
    lv_obj_add_event_cb(gpsTime, updateTime, LV_EVENT_VALUE_CHANGED, NULL);

    temp = lv_label_create(notifyBar);
    lv_label_set_text_static(temp, "--\xC2\xB0");

    sdCard = lv_label_create(notifyBar);
    if (isSdLoaded)
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
