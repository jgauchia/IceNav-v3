/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.1.5
 * @date 2023-06-04
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
static lv_obj_t *altitude;

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
static lv_obj_t *pdop_label;
static lv_obj_t *hdop_label;
static lv_obj_t *vdop_label;
static lv_obj_t *alt_label;
static lv_style_t style_radio;
static lv_style_t style_radio_chk;
static uint32_t active_gnss = 0;

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
    lv_obj_set_size(tiles, TFT_WIDTH, TFT_HEIGHT - 89);
    lv_obj_set_pos(tiles, 0, 25);
    // Main Screen Events
    lv_obj_add_event_cb(tiles, get_act_tile, LV_EVENT_SCROLL_END, NULL);
    lv_obj_add_event_cb(tiles, scroll_tile, LV_EVENT_SCROLL_BEGIN, NULL);

    // Compass Tile
    compass_heading = lv_label_create(compass_tile);
    lv_obj_set_size(compass_heading, 150, 48);
    lv_obj_set_align(compass_heading, LV_ALIGN_CENTER);
    lv_obj_set_y(compass_heading, 95);
    lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);
    lv_label_set_text(compass_heading, "-----\xC2\xB0");

    lv_obj_t *arrow_img = lv_img_create(compass_tile);
    lv_img_set_src(arrow_img, "F:/arrow.bin");
    lv_obj_align(arrow_img, LV_ALIGN_CENTER, 0, 40);

    LV_IMG_DECLARE(bruj);
    compass_img = lv_img_create(compass_tile);
    lv_img_set_src(compass_img, &bruj);
    lv_obj_align(compass_img, LV_ALIGN_CENTER, 0, 75);
    lv_img_set_pivot(compass_img, 100, 100);

    lv_obj_t *pos_img = lv_img_create(compass_tile);
    lv_img_set_src(pos_img, "F:/pin.bin");
    lv_obj_set_pos(pos_img, 15, 15);

    lv_obj_t *altit_img = lv_img_create(compass_tile);
    lv_img_set_src(altit_img, "F:/altit.bin");
    lv_obj_set_pos(altit_img, 5, 54);

    latitude = lv_label_create(compass_tile);
    lv_obj_set_size(latitude, 200, 20);
    lv_obj_set_style_text_font(latitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(latitude, Latitude_formatString(GPS.location.lat()));
    lv_obj_set_pos(latitude, 55, 12);

    longitude = lv_label_create(compass_tile);
    lv_obj_set_size(longitude, 200, 20);
    lv_obj_set_style_text_font(longitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(longitude, Longitude_formatString(GPS.location.lng()));
    lv_obj_set_pos(longitude, 55, 28);

    altitude = lv_label_create(compass_tile);
    lv_obj_set_size(altitude, 200, 20);
    lv_obj_set_style_text_font(altitude, &lv_font_montserrat_24, 0);
    lv_label_set_text(altitude, "0000 m.");
    lv_obj_set_pos(altitude, 60, 73);

    // Compass Tile Events
    lv_obj_add_event_cb(compass_heading, update_heading, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(latitude, update_latitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(longitude, update_longitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(altitude, update_altitude, LV_EVENT_VALUE_CHANGED, NULL);

    // Map Tile Events
    lv_obj_add_event_cb(map_tile, update_map, LV_EVENT_REFRESH, NULL);
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
    lv_obj_set_pos(satbar_1, 0, 175);

    satbar_2 = lv_chart_create(sat_track_tile);
    lv_obj_set_size(satbar_2, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satbar_2, 6, 0);
    lv_chart_set_range(satbar_2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser2 = lv_chart_add_series(satbar_2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_2, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_2, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satbar_2, 0, 250);

#ifdef MULTI_GNSS
    lv_style_init(&style_radio);
    lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);

    lv_style_init(&style_radio_chk);
    lv_style_set_bg_img_src(&style_radio_chk, NULL);

    lv_obj_t *gnss_sel = lv_obj_create(sat_track_tile);
    lv_obj_set_flex_flow(gnss_sel, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(gnss_sel, TFT_WIDTH, 50);
    lv_obj_set_pos(gnss_sel, 0, 330);

    lv_obj_t *gps = lv_checkbox_create(gnss_sel);
    lv_checkbox_set_text(gps, "GPS     ");
    lv_obj_add_flag(gps, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(gps, &style_radio, LV_PART_INDICATOR);
    lv_obj_add_style(gps, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t *glonass = lv_checkbox_create(gnss_sel);
    lv_checkbox_set_text(glonass, "GLONASS  ");
    lv_obj_add_flag(glonass, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(glonass, &style_radio, LV_PART_INDICATOR);
    lv_obj_add_style(glonass, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t *beidou = lv_checkbox_create(gnss_sel);
    lv_checkbox_set_text(beidou, "BEIDOU");
    lv_obj_add_flag(beidou, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(beidou, &style_radio, LV_PART_INDICATOR);
    lv_obj_add_style(beidou, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_add_state(lv_obj_get_child(gnss_sel, 0), LV_STATE_CHECKED);

    // GNSS Selection Event
    lv_obj_add_event_cb(gnss_sel, active_gnss_event, LV_EVENT_CLICKED, &active_gnss);
#endif

    // Satellite Tracking Event
    lv_obj_add_event_cb(sat_track_tile, update_sattrack, LV_EVENT_VALUE_CHANGED, NULL);
}
