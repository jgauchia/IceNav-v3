/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Notify Bar
 * @version 0.1
 * @date 2022-10-16
 */

#define UPDATE_PERIOD 10

void update_notify_bar(lv_timer_t *t);

/**
 * @brief Create a notify bar
 *
 */
void create_notify_bar()
{
    battery = lv_label_create(lv_scr_act());
    lv_obj_set_size(battery, 20, 20);
    lv_obj_set_pos(battery, TFT_WIDTH - 25, 0);
    lv_label_set_text(battery, LV_SYMBOL_BATTERY_EMPTY);

    gps_time = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_time, 100, 20);
    lv_obj_set_pos(gps_time, 0, 0);
    lv_obj_set_style_text_font(gps_time, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(gps_time, "%02d:%02d:%02d", hour(), minute(), second());

    gps_count = lv_label_create(lv_scr_act());
    lv_obj_set_size(gps_count, 50, 20);
    lv_obj_set_pos(gps_count, TFT_WIDTH - 60, 0);
    lv_label_set_text_fmt(gps_count, LV_SYMBOL_GPS "%2d", GPS.satellites.value());

    lv_timer_t *t = lv_timer_create(update_notify_bar, UPDATE_PERIOD, NULL);
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

//       char s_buf[64];
//   tft.setSwapBytes(true);
//   if (batt_level > 80 && batt_level <= 100)
//     tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_100_icon);
//   else if (batt_level <= 80 && batt_level > 60)
//     tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_75_icon);
//   else if (batt_level <= 60 && batt_level > 40)
//     tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_50_icon);
//   else if (batt_level <= 40 && batt_level > 20)
//     tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_25_icon);
//   else if (batt_level <= 20)
//     tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_0_icon);
//   tft.setSwapBytes(false);
//   sprintf(s_buf, "%3d%%", batt_level);
//   tft.drawString(s_buf, x, y + 24, 1);

}
