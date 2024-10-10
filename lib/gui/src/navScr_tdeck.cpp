/**
 * @file navScr_tdeck.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Navigation screen for lilygo T-DECK
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

 #include "navScr_tdeck.hpp"

lv_obj_t *nameNav;
lv_obj_t *latNav;
lv_obj_t *lonNav;
lv_obj_t *distNav;
lv_obj_t *arrowNav;

/**
 * @brief Navigation screen
 *
 * @param screen 
 */
void navigationScr(_lv_obj_t *screen)
{
    lv_obj_t * label;
    label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text_static(label, "Navigation to:");
    lv_obj_center(label);
    lv_obj_align(label,LV_ALIGN_TOP_LEFT,10, 20);

    nameNav = lv_label_create(screen);
    lv_obj_set_style_text_font(nameNav, fontLargeMedium, 0);
    //lv_label_set_text_fmt(nameNav, "%s","");
    lv_obj_set_width(nameNav,TFT_WIDTH-10);
    lv_obj_set_pos(nameNav,10, 37);

    label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text_static(label, "Lat:");
    lv_obj_set_pos(label, 10, 70);

    label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text_static(label, "Lon:");
    lv_obj_set_pos(label, 10, 100);

    latNav = lv_label_create(screen);
    lv_obj_set_style_text_font(latNav, fontOptions, 0);
    lv_label_set_text_fmt(latNav, "%s", "");
    lv_obj_set_pos(latNav, 60, 70);
  
    lonNav = lv_label_create(screen);
    lv_obj_set_style_text_font(lonNav, fontOptions, 0);
    lv_label_set_text_fmt(lonNav, "%s", "");
    lv_obj_set_pos(lonNav, 60, 100);

    label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, fontOptions, 0);
    lv_label_set_text_static(label, "Distance");
    lv_obj_set_pos(label,(TFT_WIDTH - 150) - ( lv_obj_get_width(label) / 2 ) , 20);

    distNav = lv_label_create(screen);
    lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
    lv_label_set_text_fmt(distNav,"%d m.", 0);
    lv_obj_set_pos(distNav,(TFT_WIDTH - 150) - ( lv_obj_get_width(distNav) / 2 ) , 40);

    arrowNav = lv_img_create(screen);
    lv_img_set_zoom(arrowNav,iconScale);
    lv_obj_update_layout(arrowNav);
    lv_obj_set_pos(arrowNav,TFT_WIDTH - 150, 70);
  
    lv_img_set_src(arrowNav, &navup);
    lv_img_set_pivot(arrowNav, 50, 50) ;
}