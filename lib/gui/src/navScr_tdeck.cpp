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
  // lv_obj_t * label;
  // label = lv_label_create(navTile);
  // lv_obj_set_style_text_font(label, fontOptions, 0);
  // lv_label_set_text_static(label, "Navigation to:");
  // lv_obj_center(label);
  // lv_obj_align(label,LV_ALIGN_TOP_LEFT,10, 20);

  // nameNav = lv_label_create(navTile);
  // lv_obj_set_style_text_font(nameNav, fontLargeMedium, 0);
  // //lv_label_set_text_fmt(nameNav, "%s","");
  // lv_obj_set_width(nameNav,TFT_WIDTH-10);
  // lv_obj_set_pos(nameNav,10, 55);

  // label = lv_label_create(navTile);
  // lv_obj_set_style_text_font(label, fontOptions, 0);
  // lv_label_set_text_static(label, "Lat:");
  // lv_obj_set_pos(label, 10, 90);

  // label = lv_label_create(navTile);
  // lv_obj_set_style_text_font(label, fontOptions, 0);
  // lv_label_set_text_static(label, "Lon:");
  // lv_obj_set_pos(label, 10, 120);

  // latNav = lv_label_create(navTile);
  // lv_obj_set_style_text_font(latNav, fontOptions, 0);
  // lv_label_set_text_fmt(latNav, "%s", "");
  // lv_obj_set_pos(latNav, 60, 90);
  
  // lonNav = lv_label_create(navTile);
  // lv_obj_set_style_text_font(lonNav, fontOptions, 0);
  // lv_label_set_text_fmt(lonNav, "%s", "");
  // lv_obj_set_pos(lonNav, 60, 120);

  // label = lv_label_create(navTile);
  // lv_obj_set_style_text_font(label, fontOptions, 0);
  // lv_label_set_text_static(label, "Distance");
  // lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

  // distNav = lv_label_create(navTile);
  // lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
  // lv_label_set_text_fmt(distNav,"%d m.", 0);
  // lv_obj_align(distNav,LV_ALIGN_CENTER, 0, -5);

  // arrowNav = lv_img_create(navTile);
  // lv_img_set_zoom(arrowNav,iconScale);
  // lv_obj_update_layout(arrowNav);
  // lv_obj_align(arrowNav,LV_ALIGN_CENTER, 0, 100);
  
  // #ifdef ENABLE_COMPASS
  //   lv_img_set_src(arrowNav, &navup);
  //   lv_img_set_pivot(arrowNav, 50, 50) ;
  // #endif
}