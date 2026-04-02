/**
 * @file widgets.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Widgets
 * @version 0.2.5
 * @date 2026-04
 */

#include "widgets.hpp"
#include "lv_subjects.hpp"

lv_obj_t *compassHeading;
lv_obj_t *compassImg;
lv_obj_t *latitude;
lv_obj_t *longitude;
lv_obj_t *altitude;
lv_obj_t *speedLabel;
lv_obj_t *sunriseLabel;
lv_obj_t *sunsetLabel;
lv_obj_t *navArrow;
lv_obj_t *zoomLabel;
lv_obj_t *zoomWidget;
lv_obj_t *mapSpeedLabel;
lv_obj_t *mapSpeed;
lv_obj_t *miniCompass;
lv_obj_t *mapCompassImg;
lv_obj_t *scaleWidget;
lv_obj_t *scaleLabel;
lv_obj_t *turnByTurn;
lv_obj_t *turnDistLabel;
lv_obj_t *turnImg;

LV_IMG_DECLARE(straight);
LV_IMG_DECLARE(slleft);
LV_IMG_DECLARE(slright);
LV_IMG_DECLARE(tleft);
LV_IMG_DECLARE(tright);
LV_IMG_DECLARE(uleft);
LV_IMG_DECLARE(uright);
LV_IMG_DECLARE(finish);

extern Gps gps;

/**
 * @brief Observer callback for compass updates
 * 
 * @details Updates both the heading label and the compass image rotation.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void compass_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    int32_t heading_val = lv_subject_get_int(subject);
    lv_label_set_text_fmt(compassHeading, "%5d\xC2\xB0", (int)heading_val);
    lv_img_set_angle(compassImg, -(heading_val * 10));
}

/**
 * @brief Observer callback for position updates (Atomic & Thread-safe)
 * 
 * @details Formats and updates Latitude or Longitude labels from scaled integers.
 * 
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void position_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = (lv_obj_t *)lv_observer_get_target_obj(observer);
    int32_t val = lv_subject_get_int(subject);
    float f = (float)val / 1000000.0f;
    
    if (subject == &subject_lat)
        lv_label_set_text(label, latFormatString(f));
    else
        lv_label_set_text(label, lonFormatString(f));
}

/**
 * @brief Edit Screen Event (drag widgets)
 * 
 * @param event Pointer to the LVGL event structure  
 */
void editWidget(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_LONG_PRESSED)
    {
        canMoveWidget = true;
    }
}

/**
 * @brief Unselect widget
 *
 * @param event Pointer to the LVGL event structure
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
            widgetSelected = false;
        }
        canMoveWidget = false;
        lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
        isScrolled = true;
    }
}

/**
 * @brief Drag widget event
 *
 * @param event Pointer to the LVGL event structure
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
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void positionWidget(lv_obj_t *screen)
{
    lv_obj_t *obj = lv_obj_create(screen);  
    lv_obj_set_height(obj, 40);
    lv_obj_set_pos(obj, coordPosX, coordPosY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    
    latitude = lv_label_create(obj);
    lv_obj_set_style_text_font(latitude, fontMedium, 0);
    lv_subject_add_observer_obj(&subject_lat, position_observer_cb, latitude, NULL);
    
    longitude = lv_label_create(obj);
    lv_obj_set_style_text_font(longitude, fontMedium, 0);
    lv_subject_add_observer_obj(&subject_lon, position_observer_cb, longitude, NULL);
    
    lv_obj_t *img = lv_img_create(obj);
    lv_img_set_src(img, positionIconFile);
    lv_img_set_zoom(img, iconScale);
    
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
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void compassWidget(lv_obj_t *screen)
{
    lv_obj_t *obj = lv_obj_create(screen);
    lv_obj_set_size(obj, 200 * scale, 200 * scale);
    lv_obj_set_pos(obj, compassPosX, compassPosY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *img = lv_img_create(obj);
    lv_img_set_src(img, arrowIconFile);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -30);
    lv_img_set_zoom(img, iconScale);
    lv_obj_update_layout(img);
    
    LV_IMG_DECLARE(bruj);
    compassImg = lv_img_create(obj);
    lv_img_set_src(compassImg, &bruj);
    lv_img_set_zoom(compassImg, iconScale);
    lv_obj_update_layout(compassImg);
    lv_obj_align_to(compassImg, obj, LV_ALIGN_CENTER, 0, 0);   
    lv_img_set_pivot(compassImg, 100, 100);
    
    compassHeading = lv_label_create(obj);
    lv_obj_set_height(compassHeading, 45);
    lv_obj_align(compassHeading, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(compassHeading, fontVeryLarge, 0);
    lv_label_set_text_static(compassHeading, "---\xC2\xB0");
    
    lv_subject_add_observer_obj(&subject_heading, compass_observer_cb, obj, NULL);
    
    objUnselect(obj);
    lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Compass_");
    lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Compass_");
}

/**
 * @brief Altitude widget
 *
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void altitudeWidget(lv_obj_t *screen)
{
    lv_obj_t *obj = lv_obj_create(screen);
    lv_obj_set_height(obj, 40 * scale);
    lv_obj_set_pos(obj, altitudePosX, altitudePosY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *img = lv_img_create(obj);
    lv_img_set_src(img, altitudeIconFile);
    lv_img_set_zoom(img, iconScale);
    lv_obj_update_layout(img);
    
    lv_obj_set_width(obj, 150);
    altitude = lv_label_create(obj);
    lv_obj_set_style_text_font(altitude, fontLargeMedium, 0);
    lv_label_bind_text(altitude, &subject_altitude, "%d m.");
    lv_obj_update_layout(altitude);
    
    objUnselect(obj);
    lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Altitude_");
    lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Altitude_");
}

/**
 * @brief Speed widget
 *
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void speedWidget(lv_obj_t *screen)
{
    lv_obj_t *obj = lv_obj_create(screen);
    lv_obj_set_height(obj, 40 * scale);
    lv_obj_set_pos(obj, speedPosX, speedPosY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *img = lv_img_create(obj);
    lv_img_set_src(img, speedIconFile);
    lv_img_set_zoom(img, iconScale);
    lv_obj_update_layout(img);
    
    lv_obj_set_width(obj, 170);
    speedLabel = lv_label_create(obj);
    lv_obj_set_style_text_font(speedLabel, fontLargeMedium, 0);
    lv_label_bind_text(speedLabel, &subject_speed, "%d Km/h");
    lv_obj_update_layout(speedLabel);
    
    objUnselect(obj);
    lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Speed_");
    lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Speed_");
}

/**
 * @brief Sunrise/Sunset widget
 *
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void sunWidget(lv_obj_t *screen)
{
    lv_obj_t *obj = lv_obj_create(screen);
    lv_obj_set_size(obj, 120, 60 * scale);
    lv_obj_set_pos(obj, sunPosX, sunPosY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *img;
    img = lv_img_create(obj);
    lv_img_set_src(img, sunriseIconFile);
    lv_img_set_zoom(img, iconScale);
    lv_obj_update_layout(img);
    
    sunriseLabel = lv_label_create(obj);
    lv_label_set_text_static(sunriseLabel, "--:--");
    lv_obj_update_layout(sunriseLabel);
    
    img = lv_img_create(obj);
    lv_img_set_src(img, sunsetIconFile);
    lv_img_set_zoom(img, iconScale);
    lv_obj_update_layout(img);
    
    sunsetLabel = lv_label_create(obj);
    lv_label_set_text_static(sunsetLabel, "--:--");
    lv_obj_update_layout(sunsetLabel);
    
    objUnselect(obj);
    lv_obj_add_event_cb(obj, editWidget, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(obj, dragWidget, LV_EVENT_PRESSING, (char *)"Sun_");
    lv_obj_add_event_cb(obj, unselectWidget, LV_EVENT_RELEASED, (char *)"Sun_");
}

/**
 * @brief Map navigation arrow widget
 *
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void navArrowWidget(lv_obj_t *screen)
{
    navArrow = lv_img_create(screen);
    lv_img_set_src(navArrow, navArrowIconFile);
    lv_obj_align(navArrow, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Map zoom widget
 *
 * @param screen Pointer to the LVGL screen object where the widget will be created
 */
void mapZoomWidget(lv_obj_t *screen)
{
    zoomWidget = lv_obj_create(screen);
    lv_obj_set_size(zoomWidget, 64, 32);
    lv_obj_clear_flag(zoomWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(zoomWidget, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(zoomWidget, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(zoomWidget, &styleMapWidget, 0);
    lv_obj_t *img = lv_img_create(zoomWidget);
    lv_img_set_src(img, zoomIconFile);
    zoomLabel = lv_label_create(zoomWidget);
    lv_obj_set_style_text_font(zoomLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(zoomLabel, "%2d", zoom);
    lv_obj_add_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Create and configure the map speed widget.
 *
 * @param screen Pointer to the LVGL screen object where the map speed widget will be created.
 */
void mapSpeedWidget(lv_obj_t *screen)
{
    mapSpeed = lv_obj_create(screen);
    lv_obj_set_size(mapSpeed, 100, 32);
    lv_obj_clear_flag(mapSpeed, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(mapSpeed, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mapSpeed, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(mapSpeed, &styleMapWidget, 0);
    lv_obj_align(mapSpeed, LV_ALIGN_BOTTOM_LEFT, 0, -1);
    lv_obj_t *img = lv_img_create(mapSpeed);
    lv_img_set_src(img, mapSpeedIconFile);
    mapSpeedLabel = lv_label_create(mapSpeed);
    lv_obj_set_style_text_font(mapSpeedLabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(mapSpeedLabel, "%3d", 0);
    lv_obj_add_flag(mapSpeed, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Observer callback for map mini-compass
 *
 * @details Updates the map mini compass image rotation based on the current heading.
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject containing the heading.
 */
static void mini_compass_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *img = (lv_obj_t *)lv_observer_get_target_obj(observer);
    if (img == NULL) 
        return;

    if (mapSet.compassRotation) 
    {
        int32_t heading_val = lv_subject_get_int(subject);
        lv_img_set_angle(img, -(heading_val * 10));
    }
}

/**
 * @brief Map compass widget
 *
 * @param screen Pointer to the LVGL screen object where the map compass widget will be created.
 */
void mapCompassWidget(lv_obj_t *screen)
{
    miniCompass = lv_obj_create(screen);
    lv_obj_set_size(miniCompass, 50, 50);
    lv_obj_clear_flag(miniCompass, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(miniCompass, &styleMapWidget, 0);
    lv_obj_set_align(miniCompass, LV_ALIGN_TOP_RIGHT);
    mapCompassImg = lv_img_create(miniCompass);
    LV_IMG_DECLARE(compassMap);
    lv_img_set_src(mapCompassImg, &compassMap);
    lv_obj_set_align(mapCompassImg, LV_ALIGN_CENTER);
    lv_obj_add_flag(miniCompass, LV_OBJ_FLAG_HIDDEN);
    lv_subject_add_observer_obj(&subject_heading, mini_compass_observer_cb, mapCompassImg, NULL);
}

/**
 * @brief Map scale widget
 *
 * @param screen Pointer to the LVGL screen object where the map scale widget will be created.
 */
void mapScaleWidget(lv_obj_t *screen)
{
    scaleWidget = lv_obj_create(screen);
    lv_obj_set_size(scaleWidget, 100, 32);
    lv_obj_clear_flag(scaleWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(scaleWidget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scaleWidget, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(scaleWidget, &styleMapWidget, 0);
    lv_obj_align(scaleWidget, LV_ALIGN_BOTTOM_LEFT, 102, -1);
    lv_obj_add_flag(scaleWidget, LV_OBJ_FLAG_HIDDEN);
    scaleLabel = lv_label_create(scaleWidget);
    lv_obj_set_style_text_font(scaleLabel, &lv_font_montserrat_12, 0);
    lv_label_set_text_fmt(scaleLabel, "%s", map_scale[zoom]);
    lv_obj_t *scale = lv_scale_create(scaleWidget);
    lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_label_show(scale, false);
    lv_obj_set_size(scale, 60, 10);
    lv_scale_set_total_tick_count(scale, 2);
    lv_scale_set_major_tick_every(scale, 2);
    lv_scale_set_range(scale, 10, 20);
}

/**
 * @brief Turn By Turn Navigation widget
 *
 * @param screen Pointer to the LVGL screen object where the navigationwidget will be created.
 */
void turnByTurnWidget(lv_obj_t *screen)
{
    turnByTurn = lv_obj_create(screen);
    lv_obj_set_size(turnByTurn, 60, 100);
    lv_obj_clear_flag(turnByTurn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(turnByTurn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(turnByTurn, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(turnByTurn, &styleMapWidget, 0);
    lv_obj_align(turnByTurn, LV_ALIGN_TOP_RIGHT, 0, 60);
    turnImg = lv_img_create(turnByTurn);
    lv_img_set_src(turnImg, &straight);
    turnDistLabel = lv_label_create(turnByTurn);
    lv_obj_set_style_text_font(turnDistLabel, &lv_font_montserrat_18, 0);
    lv_label_set_text_fmt(turnDistLabel, "%4d", 0);
    lv_obj_t *obj = lv_label_create(turnByTurn);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, 0);
    lv_label_set_text_static(obj, "m.");
    lv_obj_add_flag(turnByTurn, LV_OBJ_FLAG_HIDDEN);
}
