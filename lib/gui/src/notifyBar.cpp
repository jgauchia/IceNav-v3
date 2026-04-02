/**
 * @file notifyBar.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief LVGL - Notify Bar Screen
 * @version 0.2.5
 * @date 2026-04
 */

#include "notifyBar.hpp"
#include "tasks.hpp"
#include "lv_subjects.hpp"

lv_obj_t *mainScreen;         /**< Main screen */
lv_obj_t *notifyBarIcons;     /**< Notification bar icons container object. */
lv_obj_t *notifyBarHour;      /**< Notification bar hour display object. */

extern Storage storage;
extern Battery battery;
extern Gps gps;

/**
 * @brief Observer callback for battery icon updates
 * 
 * @details Updates the battery symbol based on the percentage from subject_battery.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void battery_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    int32_t level = lv_subject_get_int(subject);
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);

    if (level <= 500 && level > 110)
        lv_label_set_text_static(obj, "  " LV_SYMBOL_CHARGE);
    else if (level <= 110 && level > 80)
        lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_FULL);
    else if (level <= 80 && level > 60)
        lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_3);
    else if (level <= 60 && level > 40)
        lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_2);
    else if (level <= 40 && level > 20)
        lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_1);
    else if (level <= 20)
        lv_label_set_text_static(obj, LV_SYMBOL_BATTERY_EMPTY);
}

/**
 * @brief Observer callback for GPS time updates
 * 
 * @details Formats the unix timestamp into a local time string HH:MM:SS and updates the label.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing unix timestamp.
 */
static void time_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    time_t localTime = (time_t)lv_subject_get_int(subject);
    struct tm local_tm;
    struct tm *now = localtime_r(&localTime, &local_tm);

    lv_label_set_text_fmt(obj, timeFormat, now->tm_hour, now->tm_min, now->tm_sec);
}

/**
 * @brief Observer callback for GPS satellites count
 * 
 * @details Updates the satellites count label.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing satellites count.
 */
static void sats_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    int32_t sats = lv_subject_get_int(subject);
    lv_label_set_text_fmt(obj, LV_SYMBOL_GPS "%2d", (int)sats);
}

/**
 * @brief Observer callback for GPS fix mode
 * 
 * @details Updates the fix mode text indicator.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing fix mode.
 */
static void fix_mode_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    int32_t mode = lv_subject_get_int(subject);
    switch (mode)
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

/**
 * @brief LED animation callback for GPS Fix indicator
 */
static void led_anim_cb(void * var, int32_t v)
{
    lv_led_set_brightness((lv_obj_t *)var, v);
}

/**
 * @brief Observer callback for GPS fix status
 * 
 * @details Toggles LED animation based on fix status.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing boolean fix state.
 */
static void is_fixed_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    int32_t is_fixed = lv_subject_get_int(subject);
    
    // Stop any running animations on this LED
    lv_anim_delete(obj, NULL);

    if (is_fixed)
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, obj);
        lv_anim_set_exec_cb(&a, led_anim_cb);
        lv_anim_set_values(&a, 10, 255);
        lv_anim_set_time(&a, 500);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
    }
    else
    {
        lv_led_set_brightness(obj, 0);
    }
}

#ifdef ENABLE_TEMP
/**
 * @brief Observer callback for temperature updates
 * 
 * @details Formats the temperature value and updates the label.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing temperature.
 */
static void temp_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    int32_t temp_val = lv_subject_get_int(subject);
    lv_label_set_text_fmt(obj, "%02d\xC2\xB0", (int)temp_val);
}
#endif

/**
 * @brief Observer callback for WiFi status
 * 
 * @details Updates the WiFi symbol based on connection state.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing boolean WiFi state.
 */
static void wifi_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *obj = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (obj == NULL)
        return;

    int32_t is_connected = lv_subject_get_int(subject);
    
    if (is_connected)
        lv_label_set_text_static(obj, LV_SYMBOL_WIFI);
    else
        lv_label_set_text_static(obj, " ");
}

/**
 * @brief Create a notify bar.
 *
 * @details Initializes and lays out the notification bar on the main screen, adding UI elements
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

    lv_obj_add_style(notifyBarIcons, &styleTransparent, LV_PART_MAIN);
    lv_obj_add_style(notifyBarHour, &styleTransparent, LV_PART_MAIN);
    lv_obj_set_style_text_font(notifyBarIcons, fontDefault, 0);
    lv_obj_set_style_text_font(notifyBarHour, fontDefault, 0);

    gpsTime = lv_label_create(notifyBarHour);
    lv_obj_set_style_text_font(gpsTime, fontLarge, 0);
    lv_label_set_text_fmt(gpsTime, timeFormat, 0, 0, 0);
    lv_subject_add_observer_obj(&subject_time, time_observer_cb, gpsTime, NULL);
    
    wifi = lv_label_create(notifyBarIcons);
    lv_label_set_text_static(wifi, " ");
    lv_subject_add_observer_obj(&subject_wifi, wifi_observer_cb, wifi, NULL);

    #ifdef ENABLE_TEMP
        temp = lv_label_create(notifyBarIcons);
        lv_label_set_text_static(temp, "--\xC2\xB0");
        lv_subject_add_observer_obj(&subject_temp, temp_observer_cb, temp, NULL);
    #endif
    
    if (storage.getSdLoaded())
    {
        sdCard = lv_label_create(notifyBarIcons);
        lv_label_set_text_static(sdCard, LV_SYMBOL_SD_CARD);
    }

    gpsCount = lv_label_create(notifyBarIcons);
    lv_label_set_text_fmt(gpsCount, LV_SYMBOL_GPS "%2d", 0);
    lv_subject_add_observer_obj(&subject_sats, sats_observer_cb, gpsCount, NULL);
    
    gpsFix = lv_led_create(notifyBarIcons);
    lv_led_set_color(gpsFix, lv_palette_main(LV_PALETTE_RED));
    lv_obj_set_size(gpsFix, 7, 7);
    lv_led_off(gpsFix);
    lv_subject_add_observer_obj(&subject_is_fixed, is_fixed_observer_cb, gpsFix, NULL);
    
    gpsFixMode = lv_label_create(notifyBarIcons);
    lv_obj_set_style_text_font(gpsFixMode, fontSmall, 0);
    lv_label_set_text_static(gpsFixMode, "----");
    lv_subject_add_observer_obj(&subject_fix_mode, fix_mode_observer_cb, gpsFixMode, NULL);
    
    battIcon = lv_label_create(notifyBarIcons);
    lv_label_set_text_static(battIcon, LV_SYMBOL_BATTERY_EMPTY);
    lv_subject_add_observer_obj(&subject_battery, battery_observer_cb, battIcon, NULL);
}
