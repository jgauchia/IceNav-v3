/**
 * @file search_sat_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.1.3
 * @date 2023-05-10
 */

#define UPDATE_SEARCH_PERIOD 1000
static lv_obj_t *searchSat;

lv_timer_t *t;
void search_gps(lv_timer_t *t);

/**
 * @brief Create a search sat screen
 *
 */
void create_search_sat_scr()
{
    searchSat = lv_obj_create(NULL);
    lv_obj_t *label = lv_label_create(searchSat);

    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_label_set_text(label, PSTR("Searching for satellites"));
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -100);

    lv_obj_t *spinner = lv_spinner_create(searchSat, 1000, 60);
    lv_obj_set_size(spinner, 130, 130);
    lv_obj_center(spinner);

    lv_obj_t *satimg = lv_img_create(searchSat);
    lv_img_set_src(satimg, "F:/sat.bin");
    lv_obj_set_align(satimg, LV_ALIGN_CENTER);

    t = lv_timer_create(search_gps, UPDATE_SEARCH_PERIOD, NULL);
    lv_timer_ready(t);
}

/**
 * @brief Search valid GPS signal
 *
 */
void search_gps(lv_timer_t *t)
{
    if (GPS.location.isValid())
    {
        is_gps_fixed = true;
        setTime(GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.date.day(), GPS.date.month(), GPS.date.year());
        // UTC Time
        utc = now();
        // Local Time
        local = CE.toLocal(utc);

        millis_actual = millis();
        while (millis() < millis_actual + 2000)
            ;
        lv_timer_del(t);
        lv_scr_load(mainScreen);
        create_notify_bar();
    }
}
