/**
 * @file widgets.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Widgets
 * @version 0.2.1
 * @date 2025-05
 */

#include "widgets.hpp"

lv_obj_t *compassHeading;
lv_obj_t *compassImg;
lv_obj_t *latitude;
lv_obj_t *longitude;
lv_obj_t *altitude;
lv_obj_t *speedLabel;
lv_obj_t *sunriseLabel;
lv_obj_t *sunsetLabel;

extern Gps gps;

/**
 * @brief Edit Screen Event (drag widgets)
 *
 * @param event
 */
void editWidget(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  
  if (code == LV_EVENT_LONG_PRESSED)
    canMoveWidget = !canMoveWidget;
}

/**
 * @brief Unselect widget
 *
 * @param event
 */
void unselectWidget(lv_event_t *event)
{
  if (canMoveWidget)
  {
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);
    if (widgetSelected)
    {
      objUnselect(obj);
      char *widget = (char *)lv_event_get_user_data(event);
      saveWidgetPos(widget, newX, newY);
      canMoveWidget = !canMoveWidget;
      lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
      widgetSelected = false;
    }
    isScrolled = true;
  }
}

/**
 * @brief Drag widget event
 *
 * @param event
 */
void dragWidget(lv_event_t *event)
{
  if (canMoveWidget)
  {
    isScrolled = false;
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(event);
    if (!widgetSelected)
    {
      objSelect(obj);
      lv_obj_clear_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
      widgetSelected = true;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL)
      return;
    
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    
    lv_coord_t x = lv_obj_get_x(obj) + vect.x;
    lv_coord_t y = lv_obj_get_y(obj) + vect.y;
    lv_coord_t width = lv_obj_get_width(obj);
    lv_coord_t height = lv_obj_get_height(obj);
    
    // Limit drag area
    if (x > 0 && y > 0 && (x + width) < TFT_WIDTH && (y + height) < TFT_HEIGHT - 25)
    {
      lv_obj_set_pos(obj, x, y);
      newX = x;
      newY = y;
    }
  }
}

/**
 * @brief Position widget
 *
 * @param screen 
 */
void positionWidget(_lv_obj_t *screen)
{
  lv_obj_t *obj = lv_obj_create(screen);  
  lv_obj_set_height(obj,40);
  lv_obj_set_pos(obj, coordPosX, coordPosY);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  latitude = lv_label_create(obj);
  lv_obj_set_style_text_font(latitude, fontMedium, 0);
  lv_label_set_text_fmt(latitude, "%s", latFormatString(gps.gpsData.latitude));
  longitude = lv_label_create(obj);
  lv_obj_set_style_text_font(longitude, fontMedium, 0);
  lv_label_set_text_fmt(longitude, "%s", lonFormatString(gps.gpsData.longitude));
  lv_obj_t *img = lv_img_create(obj);
  lv_img_set_src(img, positionIconFile);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(latitude);
  lv_obj_update_layout(img);
  lv_obj_set_width(obj, lv_obj_get_width(latitude) + 40);
  lv_obj_align(latitude, LV_ALIGN_TOP_LEFT, 15, -12);
  lv_obj_align(longitude, LV_ALIGN_TOP_LEFT, 15, 3);
  lv_obj_align(img, LV_ALIGN_TOP_LEFT, -15, -10);
  objUnselect(obj);
  lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Coords_");
  lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Coords_");
}

/**
 * @brief Compass widget
 *
 * @param screen 
 */
void compassWidget(_lv_obj_t *screen)
{
  lv_obj_t *obj = lv_obj_create(screen);
  lv_obj_set_size(obj, 200 * scale, 200 * scale);
  lv_obj_set_pos(obj, compassPosX, compassPosY);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *img = lv_img_create(obj);
  lv_img_set_src(img, arrowIconFile);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, -30);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(img);
  LV_IMG_DECLARE(bruj);
  compassImg = lv_img_create(obj);
  lv_img_set_src(compassImg, &bruj);
  lv_img_set_zoom(compassImg,iconScale);
  lv_obj_update_layout(compassImg);
  lv_obj_align_to(compassImg, obj, LV_ALIGN_CENTER, 0, 0);   
  lv_img_set_pivot(compassImg, 100, 100) ;
  compassHeading = lv_label_create(obj);
  lv_obj_set_height(compassHeading,38);
  lv_obj_align(compassHeading, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_text_font(compassHeading, fontVeryLarge, 0);
  lv_label_set_text_static(compassHeading, "---\xC2\xB0");
  objUnselect(obj);
  lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Compass_");
  lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Compass_");
}

/**
 * @brief Compass widget
 *
 * @param screen 
 */
void altitudeWidget(_lv_obj_t *screen)
{
  lv_obj_t *obj = lv_obj_create(screen);
  lv_obj_set_height(obj, 40 * scale);
  lv_obj_set_pos(obj, altitudePosX, altitudePosY);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  altitude = lv_label_create(obj);
  lv_obj_set_style_text_font(altitude, fontLargeMedium, 0);
  lv_label_set_text_static(altitude, "0 m.");
  lv_obj_t *img = lv_img_create(obj);
  lv_img_set_src(img, altitudeIconFile);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(altitude);
  lv_obj_update_layout(img);
  lv_obj_set_width(obj, 140);
  lv_obj_align(img, LV_ALIGN_LEFT_MID, -15, 0);
  lv_obj_align(altitude, LV_ALIGN_CENTER, 15, 0);
  objUnselect(obj);
  lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Altitude_");
  lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Altitude_");
}

/**
 * @brief Speed widget
 *
 * @param screen 
 */
void speedWidget(_lv_obj_t *screen)
{
  lv_obj_t *obj = lv_obj_create(screen);
  lv_obj_set_height(obj, 40 * scale);
  lv_obj_set_pos(obj, speedPosX, speedPosY);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  speedLabel = lv_label_create(obj);
  lv_obj_set_style_text_font(speedLabel, fontLargeMedium, 0);
  lv_label_set_text_static(speedLabel, "0 Km/h");
  lv_obj_t *img = lv_img_create(obj);
  lv_img_set_src(img, speedIconFile);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(speedLabel);
  lv_obj_update_layout(img);
  lv_obj_set_width(obj, 160);
  lv_obj_align(img, LV_ALIGN_LEFT_MID, -10, 0);
  lv_obj_align(speedLabel, LV_ALIGN_CENTER, 20, 0);
  objUnselect(obj);
  lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Speed_");
  lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Speed_");
}

/**
 * @brief Sunrise/Sunset widget
 *
 * @param screen 
 */
void sunWidget(_lv_obj_t *screen)
{
  lv_obj_t *obj = lv_obj_create(screen);
  lv_obj_set_size(obj, 70, 60);
  lv_obj_set_pos(obj, sunPosX, sunPosY);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

  sunriseLabel = lv_label_create(obj);
  lv_obj_align(sunriseLabel, LV_ALIGN_TOP_RIGHT, 16, -2);
  lv_label_set_text_static(sunriseLabel, "");
  sunsetLabel = lv_label_create(obj);
  lv_obj_align(sunsetLabel,LV_ALIGN_BOTTOM_RIGHT, 16, 10);
  lv_label_set_text_static(sunsetLabel, "");

  lv_obj_t *img;
  img = lv_img_create(obj);
  lv_img_set_src(img, sunriseIconFile);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(img);
  lv_obj_align(img, LV_ALIGN_TOP_LEFT, -10, -10);
  img = lv_img_create(obj);
  lv_img_set_src(img, sunsetIconFile);
  lv_img_set_zoom(img,iconScale);
  lv_obj_update_layout(img);
  lv_obj_align(img, LV_ALIGN_BOTTOM_LEFT, -10, 10);

  objUnselect(obj);
  lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Sun_");
  lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Sun_");
}