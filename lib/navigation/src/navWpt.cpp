/**
 * @file navWpt.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Waypoint Navigation Functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

 #include "navWpt.hpp"

lv_obj_t *nameNav;
lv_obj_t *latNav;
lv_obj_t *lonNav;
lv_obj_t *distNav;

 /**
 * @brief Update navigation screen
 *
 */
void updateNavScreen()
{
  lv_label_set_text_fmt(latNav, "%s", latFormatString(loadWpt.lat));
  lv_label_set_text_fmt(lonNav, "%s", lonFormatString(loadWpt.lon));
  lv_label_set_text_fmt(nameNav, "%s",loadWpt.name);
  delete[] loadWpt.name;
}
