/**
 * @file notifyBar.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief LVGL - Notify Bar Screen
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "notifyBar.hpp"
#include "font/lv_symbol_def.h"
#include "misc/lv_event.h"

lv_obj_t *mainScreen;
lv_obj_t *notifyBarIcons;
lv_obj_t *notifyBarHour;

/**
 * @brief Update notify bar event
 *
 * @param event
 */
void updateNotifyBar(lv_event_t *event)
{
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);
  
  if (obj == gpsTime)
    lv_label_set_text_fmt(obj, timeFormat, localTime.hours, localTime.minutes, localTime.seconds);

#ifdef ENABLE_TEMP
  if (obj == temp)
    lv_label_set_text_fmt(obj, "%02d\xC2\xB0", tempValue);
#endif

  if (obj == gpsCount)
    lv_label_set_text_fmt(obj, LV_SYMBOL_GPS "%2d", gpsData.satellites);

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
    switch (gpsData.fixMode)
    {
      case gps_fix::STATUS_NONE:
        lv_label_set_text_static(obj, "----");
        break;
      case gps_fix::STATUS_STD:
        lv_label_set_text_static(obj, " 3D ");
        break;
      case gps_fix::STATUS_DGPS:
        lv_label_set_text_static(obj, "DGPS");
        break;
      case gps_fix::STATUS_PPS:
        lv_label_set_text_static(obj, "PPS");
        break;
      case gps_fix::STATUS_RTK_FLOAT:
        lv_label_set_text_static(obj, "RTK");
        break;
      case gps_fix::STATUS_RTK_FIXED:
        lv_label_set_text_static(obj, "RTK");
        break;
      case gps_fix::STATUS_TIME_ONLY: 
        lv_label_set_text_static(obj, "TIME");
        break;       
      case gps_fix::STATUS_EST:
        lv_label_set_text_static(obj, "EST");
        break;  
    }
  }

  if (obj == wifi)
  {
    if (WiFi.status() == WL_CONNECTED)
      lv_label_set_text_static(obj, LV_SYMBOL_WIFI);
    else
      lv_label_set_text_static(obj," ");
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
  lv_obj_send_event(wifi, LV_EVENT_VALUE_CHANGED, NULL);
 
  if (isGpsFixed)
    lv_led_toggle(gpsFix);
  else
    lv_led_off(gpsFix);

  #ifdef ENABLE_TEMP
  tempValue = (uint8_t)(bme.readTemperature());
  if (tempValue != tempOld)
  {
    lv_obj_send_event(temp, LV_EVENT_VALUE_CHANGED, NULL);
    tempOld = tempValue;
  }
  #endif

  battLevel = batteryRead();
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
  notifyBarIcons = lv_obj_create(mainScreen);
  lv_obj_set_size(notifyBarIcons, (TFT_WIDTH / 3) * 2 , 24);
  lv_obj_set_pos(notifyBarIcons, (TFT_WIDTH / 3) + 1, 0);
  lv_obj_set_flex_flow(notifyBarIcons, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(notifyBarIcons, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(notifyBarIcons, LV_OBJ_FLAG_SCROLLABLE);

  notifyBarHour = lv_obj_create(mainScreen);
  lv_obj_set_size(notifyBarHour, TFT_WIDTH / 3 , 24);
  lv_obj_set_pos(notifyBarHour, 0, 0);
  lv_obj_set_flex_flow(notifyBarHour, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(notifyBarHour, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(notifyBarHour, LV_OBJ_FLAG_SCROLLABLE);

  static lv_style_t styleBar;
  lv_style_init(&styleBar);
  lv_style_set_bg_opa(&styleBar, LV_OPA_0);
  lv_style_set_border_opa(&styleBar, LV_OPA_0);
  lv_style_set_text_font(&styleBar, fontDefault);
  lv_obj_add_style(notifyBarIcons, &styleBar, LV_PART_MAIN);
  lv_obj_add_style(notifyBarHour, &styleBar, LV_PART_MAIN);
  
  lv_obj_t *label;
  
  gpsTime = lv_label_create(notifyBarHour);
  lv_obj_set_style_text_font(gpsTime, fontLarge, 0);
  lv_label_set_text_fmt(gpsTime, timeFormat, 0, 0, 0);
  lv_obj_add_event_cb(gpsTime, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);
 
  wifi = lv_label_create(notifyBarIcons);
  lv_label_set_text_static(wifi, " ");
  lv_obj_add_event_cb(wifi, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);

  #ifdef ENABLE_TEMP
  temp = lv_label_create(notifyBarIcons);
  lv_label_set_text_static(temp, "--\xC2\xB0");
  lv_obj_add_event_cb(temp, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);
  #endif
  
  if (isSdLoaded)
  {
    sdCard = lv_label_create(notifyBarIcons);
    lv_label_set_text_static(sdCard, LV_SYMBOL_SD_CARD);
  }

  gpsCount = lv_label_create(notifyBarIcons);
  lv_label_set_text_fmt(gpsCount, LV_SYMBOL_GPS "%2d", 0);
  lv_obj_add_event_cb(gpsCount, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);
  
  gpsFix = lv_led_create(notifyBarIcons);
  lv_led_set_color(gpsFix, lv_palette_main(LV_PALETTE_RED));
  lv_obj_set_size(gpsFix, 7, 7);
  lv_led_off(gpsFix);
  
  gpsFixMode = lv_label_create(notifyBarIcons);
  lv_obj_set_style_text_font(gpsFixMode, fontSmall, 0);
  lv_label_set_text_static(gpsFixMode, "----");
  lv_obj_add_event_cb(gpsFixMode, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);
  
  battery = lv_label_create(notifyBarIcons);
  lv_label_set_text_static(battery, LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_add_event_cb(battery, updateNotifyBar, LV_EVENT_VALUE_CHANGED, NULL);
  
  lv_timer_t *timerNotifyBar = lv_timer_create(updateNotifyBarTimer, UPDATE_NOTIFY_PERIOD, NULL);
  lv_timer_ready(timerNotifyBar);
}
