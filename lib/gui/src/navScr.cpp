/**
 * @file navScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Navigation screen 
 * @version 0.2.0
 * @date 2025-04
 */

 #include "navScr.hpp"

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
#ifndef TDECK_ESP32S3
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
  lv_label_set_long_mode(nameNav, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(nameNav,TFT_WIDTH-10);
  lv_obj_set_pos(nameNav,10, 55);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lat:");
  lv_obj_set_pos(label, 10, 90);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lon:");
  lv_obj_set_pos(label, 10, 120);

  latNav = lv_label_create(screen);
  lv_obj_set_style_text_font(latNav, fontOptions, 0);
  lv_label_set_text_fmt(latNav, "%s", "");
  lv_obj_set_pos(latNav, 60, 90);
  
  lonNav = lv_label_create(screen);
  lv_obj_set_style_text_font(lonNav, fontOptions, 0);
  lv_label_set_text_fmt(lonNav, "%s", "");
  lv_obj_set_pos(lonNav, 60, 120);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Distance");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

  distNav = lv_label_create(screen);
  lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
  lv_label_set_text_fmt(distNav,"%d m.", 0);
  lv_obj_align(distNav,LV_ALIGN_CENTER, 0, -5);

  arrowNav = lv_img_create(screen);
  lv_img_set_zoom(arrowNav,iconScale);
  lv_obj_update_layout(arrowNav);
  lv_obj_align(arrowNav,LV_ALIGN_CENTER, 0, 100);

  lv_img_set_src(arrowNav, &navup);
  lv_img_set_pivot(arrowNav, 50, 50) ;
}
#endif

#ifdef TDECK_ESP32S3
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
  lv_label_set_long_mode(nameNav, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(nameNav,TFT_WIDTH-10);
  lv_obj_set_pos(nameNav,10, 37);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lat:");
  lv_obj_set_pos(label, 10, 70);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Lon:");
  lv_obj_set_pos(label, 10, 90);

  latNav = lv_label_create(screen);
  lv_obj_set_style_text_font(latNav, fontOptions, 0);
  lv_label_set_text_fmt(latNav, "%s", "");
  lv_obj_set_pos(latNav, 60, 70);

  lonNav = lv_label_create(screen);
  lv_obj_set_style_text_font(lonNav, fontOptions, 0);
  lv_label_set_text_fmt(lonNav, "%s", "");
  lv_obj_set_pos(lonNav, 60, 90);

  label = lv_label_create(screen);
  lv_obj_set_style_text_font(label, fontOptions, 0);
  lv_label_set_text_static(label, "Distance");
  lv_obj_set_pos(label,10, 120);

  distNav = lv_label_create(screen);
  lv_obj_set_style_text_font(distNav, fontVeryLarge, 0);
  lv_label_set_text_fmt(distNav,"%d m.", 0);
  lv_obj_set_pos(distNav,10, 140);

  arrowNav = lv_img_create(screen);
  lv_obj_set_pos(arrowNav,TFT_WIDTH - 100, 35);

  lv_img_set_src(arrowNav, &navup);
  lv_img_set_pivot(arrowNav, 50, 50) ;
}
#endif