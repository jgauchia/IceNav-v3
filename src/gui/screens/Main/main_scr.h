/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Main Screen
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief Main Screen Tiles
 *
 */
static lv_obj_t *compassTile;
static lv_obj_t *navTile;
static lv_obj_t *mapTile;
static lv_obj_t *satTrackTile;

/**
 * @brief Compass Tile screen objects
 *
 */
static lv_obj_t *compassHeading;
static lv_obj_t *compassImg;
static lv_obj_t *latitude;
static lv_obj_t *longitude;
static lv_obj_t *altitude;
static lv_obj_t *speedLabel;

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
lv_obj_t *pdopLabel;
lv_obj_t *hdopLabel;
lv_obj_t *vdopLabel;
lv_obj_t *altLabel;
static lv_style_t styleRadio;
static lv_style_t styleRadioChk;
static uint32_t activeGnss = 0;

/**
 * @brief Main screen events include
 *
 */
#include "gui/screens/Main/events/main_scr.h"
#include "gui/screens/Main/events/compass.h"
#include "gui/screens/Main/events/map.h"
#include "gui/screens/Main/events/sattrack.h"

/**
 * @brief Create Main Screen
 *
 */
void createMainScr()
{
    mainScreen = lv_obj_create(NULL);

    // Main Screen Tiles
    tiles = lv_tileview_create(mainScreen);
    compassTile = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_RIGHT);
    mapTile = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    navTile = lv_tileview_add_tile(tiles, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    satTrackTile = lv_tileview_add_tile(tiles, 3, 0, LV_DIR_LEFT);
    lv_obj_set_size(tiles, TFT_WIDTH, TFT_HEIGHT - 25);
    lv_obj_set_pos(tiles, 0, 25);
    static lv_style_t style_scroll;
    lv_style_init(&style_scroll);
    lv_style_set_bg_color(&style_scroll, lv_color_hex(0xFFFFFF));
    lv_obj_add_style(tiles, &style_scroll, LV_PART_SCROLLBAR);
    // Main Screen Events
    lv_obj_add_event_cb(tiles, getActTile, LV_EVENT_SCROLL_END, NULL);
    lv_obj_add_event_cb(tiles, scrollTile, LV_EVENT_SCROLL_BEGIN, NULL);

    // Compass Tile

    // Compass Widget
    lv_obj_t *compassWidget = lv_obj_create(compassTile);
    lv_obj_set_size(compassWidget, 200, 200);
    lv_obj_set_pos(compassWidget, compassPosX, compassPosY);
    lv_obj_clear_flag(compassWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *arrowImg = lv_img_create(compassWidget);
    lv_img_set_src(arrowImg, "F:/arrow.bin");
    lv_obj_align(arrowImg, LV_ALIGN_CENTER, 0, -30);

    LV_IMG_DECLARE(bruj);
    compassImg = lv_img_create(compassWidget);
    lv_img_set_src(compassImg, &bruj);
    lv_obj_align(compassImg, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_pivot(compassImg, 100, 100);
    compassHeading = lv_label_create(compassWidget);
    lv_obj_set_size(compassHeading, 150, 38);
    lv_obj_align(compassHeading, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(compassHeading, &lv_font_montserrat_48, 0);
    lv_label_set_text_static(compassHeading, "-----\xC2\xB0");
    objUnselect(compassWidget);
    lv_obj_add_event_cb(compassWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Compass_");
    lv_obj_add_event_cb(compassWidget, unselectWidget, LV_EVENT_RELEASED, NULL);

    // Position widget
    lv_obj_t *positionWidget = lv_obj_create(compassTile);
    lv_obj_set_size(positionWidget, 190, 40);
    lv_obj_set_pos(positionWidget, coordPosX, coordPosY);
    lv_obj_clear_flag(positionWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *posImg = lv_img_create(positionWidget);
    lv_img_set_src(posImg, "F:/pin.bin");
    lv_obj_align(posImg, LV_ALIGN_LEFT_MID, -15, 0);
    latitude = lv_label_create(positionWidget);
    lv_obj_set_style_text_font(latitude, &lv_font_montserrat_16, 0);
    lv_label_set_text_static(latitude, latitudeFormatString(GPS.location.lat()));
    lv_obj_align(latitude, LV_ALIGN_TOP_LEFT, 25, -12);
    longitude = lv_label_create(positionWidget);
    lv_obj_set_style_text_font(longitude, &lv_font_montserrat_16, 0);
    lv_label_set_text_static(longitude, longitudeFormatString(GPS.location.lng()));
    lv_obj_align(longitude, LV_ALIGN_TOP_LEFT, 25, 3);
    objUnselect(positionWidget);
    lv_obj_add_event_cb(positionWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Coords_");
    lv_obj_add_event_cb(positionWidget, unselectWidget, LV_EVENT_RELEASED, NULL);

    // Altitude widget
    lv_obj_t *altitudeWidget = lv_obj_create(compassTile);
    lv_obj_set_size(altitudeWidget, 140, 40);
    lv_obj_set_pos(altitudeWidget, altitudePosX, altitudePosY);
    lv_obj_clear_flag(altitudeWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *altitImg = lv_img_create(altitudeWidget);
    lv_img_set_src(altitImg, "F:/altit.bin");
    lv_obj_align(altitImg, LV_ALIGN_LEFT_MID, -15, 0);
    altitude = lv_label_create(altitudeWidget);
    lv_obj_set_style_text_font(altitude, &lv_font_montserrat_24, 0);
    lv_label_set_text_static(altitude, "0000 m.");
    lv_obj_align(altitude, LV_ALIGN_CENTER, 10, 0);
    objUnselect(altitudeWidget);
    lv_obj_add_event_cb(altitudeWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Altitude_");
    lv_obj_add_event_cb(altitudeWidget, unselectWidget, LV_EVENT_RELEASED, NULL);

    // Speed widget
    lv_obj_t *speedWidget = lv_obj_create(compassTile);
    lv_obj_set_size(speedWidget, 190, 40);
    lv_obj_set_pos(speedWidget, speedPosX, speedPosY);
    lv_obj_clear_flag(speedWidget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *speedImg = lv_img_create(speedWidget);
    lv_img_set_src(speedImg, "F:/speed.bin");
    lv_obj_align(speedImg, LV_ALIGN_LEFT_MID, -10, 0);
    speedLabel = lv_label_create(speedWidget);
    lv_obj_set_style_text_font(speedLabel, &lv_font_montserrat_24, 0);
    lv_label_set_text_static(speedLabel, "0 Km/h");
    lv_obj_align(speedLabel, LV_ALIGN_CENTER, 0, 0);
    objUnselect(speedWidget);
    lv_obj_add_event_cb(speedWidget, dragWidget, LV_EVENT_PRESSING, (char *)"Speed_");
    lv_obj_add_event_cb(speedWidget, unselectWidget, LV_EVENT_RELEASED, NULL);

    // Compass Tile Events
    lv_obj_add_event_cb(compassHeading, updateHeading, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(latitude, updateLatitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(longitude, updateLongitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(altitude, updateAltitude, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(speedLabel, updateSpeed, LV_EVENT_VALUE_CHANGED, NULL);

    // Map Tile Events
    lv_obj_add_event_cb(mapTile, updateMap, LV_EVENT_REFRESH, NULL);
    lv_obj_add_event_cb(mainScreen, getZoomValue, LV_EVENT_GESTURE, NULL);

    // Navigation Tile
    // TODO
    lv_obj_t *todolabel = lv_label_create(navTile);
    lv_obj_set_style_text_font(todolabel, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(todolabel, "NAVIGATION SCREEN -> TODO");
    lv_obj_center(todolabel);

    // Navitagion Tile Events
    // TODO

    // Satellite Tracking Tile
    lv_obj_t *infoGrid = lv_obj_create(satTrackTile);
    lv_obj_set_size(infoGrid, 90, 175);
    lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(infoGrid, 5, 0);
    lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_COLUMN);
    static lv_style_t styleGrid;
    lv_style_init(&styleGrid);
    lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
    lv_style_set_border_opa(&styleGrid, LV_OPA_0);
    lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);

    pdopLabel = lv_label_create(infoGrid);
    lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", pdop.value());

    hdopLabel = lv_label_create(infoGrid);
    lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", hdop.value());

    vdopLabel = lv_label_create(infoGrid);
    lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", vdop.value());

    altLabel = lv_label_create(infoGrid);
    lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", (int)GPS.altitude.meters());

    satelliteBar1 = lv_chart_create(satTrackTile);
    lv_obj_set_size(satelliteBar1, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satelliteBar1, 6, 0);
    lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satelliteBar1, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satelliteBar1, 0, 175);

    satelliteBar2 = lv_chart_create(satTrackTile);
    lv_obj_set_size(satelliteBar2, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satelliteBar2, 6, 0);
    lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satelliteBar2, 0, 260);

#ifdef MULTI_GNSS
    lv_style_init(&styleRadio);
    lv_style_set_radius(&styleRadio, LV_RADIUS_CIRCLE);

    lv_style_init(&styleRadioChk);
    lv_style_set_bg_image_src(&styleRadioChk, NULL);

    lv_obj_t *gnssSel = lv_obj_create(satTrackTile);
    lv_obj_set_flex_flow(gnssSel, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(gnssSel, TFT_WIDTH, 50);
    lv_obj_set_pos(gnssSel, 0, 330);
    static lv_style_t styleSel;
    lv_style_init(&styleSel);
    lv_style_set_bg_opa(&styleSel, LV_OPA_0);
    lv_style_set_border_opa(&styleSel, LV_OPA_0);
    lv_obj_add_style(gnssSel, &styleSel, LV_PART_MAIN);

    lv_obj_t *gps = lv_checkbox_create(gnssSel);
    lv_checkbox_set_text_static(gps, "GPS     ");
    lv_obj_add_flag(gps, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(gps, &styleRadio, LV_PART_INDICATOR);
    lv_obj_add_style(gps, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t *glonass = lv_checkbox_create(gnssSel);
    lv_checkbox_set_text_static(glonass, "GLONASS  ");
    lv_obj_add_flag(glonass, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(glonass, &styleRadio, LV_PART_INDICATOR);
    lv_obj_add_style(glonass, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t *beidou = lv_checkbox_create(gnssSel);
    lv_checkbox_set_text_static(beidou, "BEIDOU");
    lv_obj_add_flag(beidou, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(beidou, &styleRadio, LV_PART_INDICATOR);
    lv_obj_add_style(beidou, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_add_state(lv_obj_get_child(gnssSel, 0), LV_STATE_CHECKED);

    // GNSS Selection Event
    lv_obj_add_event_cb(gnssSel, activeGnssEvent, LV_EVENT_CLICKED, &activeGnss);
#endif

    // Satellite Tracking Event
    lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);
}
