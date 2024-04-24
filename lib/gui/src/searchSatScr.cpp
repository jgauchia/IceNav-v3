/**
 * @file searchSatScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - GPS satellite search screen
 * @version 0.1.8
 * @date 2024-04
 */

#include "searchSatScr.hpp"

static unsigned long millisActual = 0;

/**
 * @brief Search valid GPS signal
 *
 */
void searchGPS(lv_timer_t *searchTimer)
{
    if (GPS.location.isValid())
    {
        isGpsFixed = true;
       
        millisActual = millis();
        while (millis() < millisActual + 2000)
            ;
        lv_timer_del(searchTimer);
        loadMainScreen();
    }
}

/**
 * @brief Create Satellite Search Screen
 *
 */
void createSearchSatScr()
{
    searchSatScreen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(searchSatScreen);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_label_set_text(label, textSearch);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -100);

    lv_obj_t *spinner = lv_spinner_create(searchSatScreen);
    lv_obj_set_size(spinner, 130, 130);
    lv_spinner_set_anim_params(spinner, 2000, 200);
    lv_obj_center(spinner);

    lv_obj_t *satImg = lv_img_create(searchSatScreen);
    lv_img_set_src(satImg, satIconFile);
    lv_obj_set_align(satImg, LV_ALIGN_CENTER);

    searchTimer = lv_timer_create(searchGPS, 1000, NULL);
    lv_timer_ready(searchTimer);
}
