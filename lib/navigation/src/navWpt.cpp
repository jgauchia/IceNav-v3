/**
 * @file navWpt.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Waypoint Navigation Functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

 #include "navWpt.hpp"

 /**
 * @brief Update navigation screen
 *
 */
void updateNavScreen()
{
  lv_label_set_text_static(latNav, latFormatString(addWpt.lat));
  lv_label_set_text_static(lonNav, lonFormatString(addWpt.lon));
}
