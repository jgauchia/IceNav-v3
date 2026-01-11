/**
 * @file navScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Navigation screen 
 * @version 0.2.4
 * @date 2025-12
 */

 #include "navScr.hpp"

lv_obj_t *nameNav;
lv_obj_t *latNav;
lv_obj_t *lonNav;
lv_obj_t *distNav;
lv_obj_t *arrowNav;

/**
 * @brief Creates and initializes the navigation screen, adding labels and widgets for navigation information display.
 *
 * @param screen LVGL screen object to which the navigation UI elements are added.
 */
void navigationScr(_lv_obj_t *screen)
{
    lv_obj_t *label;
    
    label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text_static(label, "Navigation to:");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 20);

    nameNav = lv_label_create(screen);
    lv_obj_set_style_text_font(nameNav, fontLargeMedium, 0);
    lv_label_set_long_mode(nameNav, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(nameNav, TFT_WIDTH - 10);

    lv_obj_t *labelLat = lv_label_create(screen);
    lv_obj_set_style_text_font(labelLat, fontOptions, 0);
    lv_label_set_text_static(labelLat, "Lat:");

    lv_obj_t *labelLon = lv_label_create(screen);
    lv_obj_set_style_text_font(labelLon, fontOptions, 0);
    lv_label_set_text_static(labelLon, "Lon:");

    latNav = lv_label_create(screen);
    lv_obj_set_style_text_font(latNav, fontOptions, 0);
    lv_label_set_text_fmt(latNav, "%s", "");

    lonNav = lv_label_create(screen);
    lv_obj_set_style_text_font(lonNav, fontOptions, 0);
    lv_label_set_text_fmt(lonNav, "%s", "");

    lv_obj_t *labelDist = lv_label_create(screen);
    lv_obj_set_style_text_font(labelDist, fontOptions, 0);
    lv_label_set_text_static(labelDist, "Distance");

    distNav = lv_label_create(screen);
    lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
    lv_label_set_text_fmt(distNav, "%d m.", 0);

    arrowNav = lv_img_create(screen);
    LV_IMG_DECLARE(navup);
    lv_img_set_src(arrowNav, &navup);
    lv_img_set_pivot(arrowNav, 50, 50);

#ifdef TDECK_ESP32S3
    lv_obj_set_pos(nameNav, 10, 37);
    lv_obj_set_pos(labelLat, 10, 70);
    lv_obj_set_pos(labelLon, 10, 90);
    lv_obj_set_pos(latNav, 60, 70);
    lv_obj_set_pos(lonNav, 60, 90);
    lv_obj_set_pos(labelDist, 10, 120);
    lv_obj_set_pos(distNav, 10, 140);
    lv_obj_set_pos(arrowNav, TFT_WIDTH - 100, 35);
#else
    lv_obj_set_pos(nameNav, 10, 55);
    lv_obj_set_pos(labelLat, 10, 90);
    lv_obj_set_pos(labelLon, 10, 120);
    lv_obj_set_pos(latNav, 60, 90);
    lv_obj_set_pos(lonNav, 60, 120);
    lv_obj_align(labelDist, LV_ALIGN_CENTER, 0, -50);
    lv_obj_align(distNav, LV_ALIGN_CENTER, 0, -5);
    lv_obj_align(arrowNav, LV_ALIGN_CENTER, 0, 100);
    lv_img_set_zoom(arrowNav, iconScale);
#endif
}
