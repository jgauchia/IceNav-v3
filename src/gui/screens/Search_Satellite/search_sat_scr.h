/**
 * @file search_sat_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.1.8
 * @date 2024-04
 */

#define UPDATE_SEARCH_PERIOD 1000
static lv_obj_t *searchSat;

lv_timer_t *searchTimer;
void searchGPS(lv_timer_t *searchTimer);
void loadMainScreen();

/**
 * @brief Create search sat screen
 *
 */
void createSearchSatScr()
{
    searchSat = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(searchSat);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_label_set_text(label, PSTR("Searching for satellites"));
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -100);

    lv_obj_t *spinner = lv_spinner_create(searchSat);
    lv_obj_set_size(spinner, 130, 130);
    lv_spinner_set_anim_params(spinner, 2000, 200);
    lv_obj_center(spinner);

    lv_obj_t *satimg = lv_img_create(searchSat);
    lv_img_set_src(satimg, "F:/sat.bin");
    lv_obj_set_align(satimg, LV_ALIGN_CENTER);

    searchTimer = lv_timer_create(searchGPS, UPDATE_SEARCH_PERIOD, NULL);
    lv_timer_ready(searchTimer);
}

/**
 * @brief Search valid GPS signal
 *
 */
void searchGPS(lv_timer_t *searchTimer)
{
    if (GPS.location.isValid())
    {
        isGpsFixed = true;
        setTime(GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.date.day(), GPS.date.month(), GPS.date.year());
        // UTC Time
        utc = now();
        // Local Time
        local = CE.toLocal(utc);

        millisActual = millis();
        while (millis() < millisActual + 2000)
            ;
        lv_timer_del(searchTimer);
        loadMainScreen();
    }
}
