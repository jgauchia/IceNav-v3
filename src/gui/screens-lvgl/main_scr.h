/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.1
 * @date 2022-10-16
 */

/**
 * @brief Main Screen Tiles
 *
 */
static lv_obj_t *compass_tile;
static lv_obj_t *map_tile;
static lv_obj_t *sat_track_tile;

/**
 * @brief Compass Tile screen objects
 *
 */
static lv_obj_t *compass_heading;
static lv_obj_t *compass_img;
static lv_obj_t *latitude;
static lv_obj_t *longitude;

/**
 * @brief Map View Tile screen objects
 *
 */
static lv_obj_t *zoom_label;
static lv_obj_t *zoom_slider;

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
static lv_obj_t *pdop_label;
static lv_obj_t *hdop_label;
static lv_obj_t *vdop_label;
static lv_obj_t *alt_label;

/**
 * @brief Main screen events include
 *
 */
#include "gui/events/main_screen.h"
#include "gui/events/compass.h"
#include "gui/events/map.h"
#include "gui/events/sattrack.h"

/**
 * @brief Create Main Screen
 *
 */
void create_main_scr()
{
    mainScreen = lv_obj_create(NULL);

    // Main Screen Tiles
    tiles = lv_tileview_create(mainScreen);
    compass_tile = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_RIGHT);
    map_tile = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    sat_track_tile = lv_tileview_add_tile(tiles, 2, 0, LV_DIR_LEFT);
    lv_obj_set_size(tiles, TFT_WIDTH, TFT_HEIGHT - 84);
    lv_obj_set_pos(tiles, 0, 20);
    // Main Screen Events
    lv_obj_add_event_cb(tiles, get_act_tile, LV_EVENT_SCROLL_END, NULL);
    // Main Screen update
    lv_timer_t *timer_main_scr = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(timer_main_scr);

    // Compass Tile
#ifdef ENABLE_COMPASS
    compass_heading = lv_label_create(compass_tile);
    lv_obj_set_size(compass_heading, 150, 48);
    lv_obj_set_align(compass_heading, LV_ALIGN_CENTER);
    lv_obj_set_y(compass_heading, 35);
    lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);

    lv_obj_t *arrow_img = lv_img_create(compass_tile);
    lv_img_set_src(arrow_img, "F:/arrow.bin");
    lv_obj_align(arrow_img, LV_ALIGN_CENTER, 0, -20);

    LV_IMG_DECLARE(bruj);
    compass_img = lv_img_create(compass_tile);
    lv_img_set_src(compass_img, &bruj);
    lv_obj_align(compass_img, LV_ALIGN_CENTER, 0, 15);
    lv_img_set_pivot(compass_img, 100, 100);
#endif

    lv_obj_t *pos_img = lv_img_create(compass_tile);
    lv_img_set_src(pos_img, "F:/pin.bin");
    lv_obj_set_pos(pos_img, 5, 10);

    latitude = lv_label_create(compass_tile);
    lv_obj_set_size(latitude, 200, 20);
    lv_obj_set_style_text_font(latitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(latitude, Latitude_formatString(GPS.location.lat()));
    lv_obj_set_pos(latitude, 45, 7);

    longitude = lv_label_create(compass_tile);
    lv_obj_set_size(longitude, 200, 20);
    lv_obj_set_style_text_font(longitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(longitude, Longitude_formatString(GPS.location.lng()));
    lv_obj_set_pos(longitude, 45, 23);
    // Compass Tile Events
    lv_obj_add_event_cb(compass_heading, update_heading, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(latitude, update_latitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(longitude, update_longitude, LV_EVENT_VALUE_CHANGED, NULL);

    // Map Tile
    zoom_label = lv_label_create(map_tile);
    lv_obj_set_style_text_font(zoom_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", DEF_ZOOM);
    lv_obj_align(zoom_label, LV_ALIGN_TOP_MID, 0, 10);

    // Map Tile Events
    lv_obj_add_event_cb(map_tile, draw_map, LV_EVENT_DRAW_POST_END, NULL);
    lv_obj_add_event_cb(map_tile, update_map, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(mainScreen, get_zoom_value, LV_EVENT_GESTURE, NULL);

    // Satellite Tracking Tile
    pdop_label = lv_label_create(sat_track_tile);
    lv_obj_set_size(pdop_label, 55, 40);
    lv_obj_set_style_text_font(pdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(pdop_label, "PDOP:\n%s", pdop.value());
    lv_obj_set_pos(pdop_label, 5, 15);

    hdop_label = lv_label_create(sat_track_tile);
    lv_obj_set_size(hdop_label, 55, 40);
    lv_obj_set_style_text_font(hdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(hdop_label, "HDOP:\n%s", hdop.value());
    lv_obj_set_pos(hdop_label, 5, 50);

    vdop_label = lv_label_create(sat_track_tile);
    lv_obj_set_size(vdop_label, 55, 40);
    lv_obj_set_style_text_font(vdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(vdop_label, "VDOP:\n%s", vdop.value());
    lv_obj_set_pos(vdop_label, 5, 85);

    alt_label = lv_label_create(sat_track_tile);
    lv_obj_set_size(alt_label, 55, 80);
    lv_obj_set_style_text_font(alt_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(alt_label, "ALT:\n%4dm.", (int)GPS.altitude.meters());
    lv_obj_set_pos(alt_label, 5, 120);

    satbar_1 = lv_chart_create(sat_track_tile);
    lv_obj_set_size(satbar_1, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satbar_1, 6, 0);
    lv_chart_set_range(satbar_1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser1 = lv_chart_add_series(satbar_1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_1, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_1, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satbar_1, 0, 180);

    satbar_2 = lv_chart_create(sat_track_tile);
    lv_obj_set_size(satbar_2, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satbar_2, 6, 0);
    lv_chart_set_range(satbar_2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser2 = lv_chart_add_series(satbar_2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_2, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_2, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satbar_2, 0, 265);
    // Satellite Tracking Event
    lv_obj_add_event_cb(sat_track_tile, update_sattrack, LV_EVENT_VALUE_CHANGED, NULL);
}
