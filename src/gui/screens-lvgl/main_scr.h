/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-16
 */
int heading = 0;

#define UPDATE_MAINSCR_PERIOD 30
void update_main_screen(lv_timer_t *t);
void lvgl_set_resolution(int width, int height);

static lv_obj_t *compass_heading;
static lv_obj_t *compass_img;
static lv_obj_t *latitude;
static lv_obj_t *longitude;
static lv_obj_t *zoom_label;
static lv_obj_t *pdop_label;
static lv_obj_t *hdop_label;
static lv_obj_t *vdop_label;
static lv_obj_t *alt_label;
static lv_obj_t *map_tile;
lv_obj_t *satbar_1;
lv_obj_t *satbar_2;
lv_chart_series_t *satbar_ser1;
lv_chart_series_t *satbar_ser2;

static lv_timer_t *timer_main_scr;
static lv_obj_t *zoombox;
TFT_eSprite sprArrow = TFT_eSprite(&tft);
TFT_eSprite sprSat = TFT_eSprite(&tft);

/**
 * @brief Active Tile in TileView control
 *
 */
uint16_t act_tile = 0;
enum tilename
{
    COMPASS,
    MAP,
    SATTRACK,
};

#include "gui/events/compass_scr.h"
#include "gui/events/map_scr.h"
#include "gui/events/main_scr.h"

/**
 * @brief Create a main screen
 *
 */
void create_main_scr()
{
    mainScreen = lv_obj_create(NULL);

    // Main Screen Tiles
    tiles = lv_tileview_create(mainScreen);
    lv_obj_t *compass_tile = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_RIGHT);
    map_tile = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *sat_track = lv_tileview_add_tile(tiles, 2, 0, LV_DIR_LEFT);
    lv_obj_set_size(tiles, TFT_WIDTH, TFT_HEIGHT - 84);
    lv_obj_set_pos(tiles, 0, 20);
    lv_obj_add_event_cb(tiles, get_act_tile, LV_EVENT_SCROLL_END, NULL);

    // Compass Tile
#ifdef ENABLE_COMPASS
    compass_heading = lv_label_create(compass_tile);
    lv_obj_set_size(compass_heading, 150, 48);
    lv_obj_set_align(compass_heading, LV_ALIGN_CENTER);
    lv_obj_set_y(compass_heading, 35);
    lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);
    lv_obj_add_event_cb(compass_heading, update_heading, LV_EVENT_VALUE_CHANGED, NULL);

    LV_IMG_DECLARE(arrow);
    lv_obj_t *arrow_img = lv_img_create(compass_tile);
    lv_img_set_src(arrow_img, &arrow);
    lv_obj_align(arrow_img, LV_ALIGN_CENTER, 0, -20);

    LV_IMG_DECLARE(bruj);
    compass_img = lv_img_create(compass_tile);
    lv_img_set_src(compass_img, &bruj);
    lv_obj_align(compass_img, LV_ALIGN_CENTER, 0, 15);
    lv_img_set_pivot(compass_img, 100, 100);
#endif

    LV_IMG_DECLARE(position);
    lv_obj_t *pos_img = lv_img_create(tiles);
    lv_img_set_src(pos_img, &position);
    lv_obj_set_pos(pos_img, 5, 10);

    latitude = lv_label_create(tiles);
    lv_obj_set_size(latitude, 200, 20);
    lv_obj_set_style_text_font(latitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(latitude, Latitude_formatString(GPS.location.lat()));
    lv_obj_set_pos(latitude, 45, 7);
    lv_obj_add_event_cb(latitude, update_latitude, LV_EVENT_VALUE_CHANGED, NULL);

    longitude = lv_label_create(tiles);
    lv_obj_set_size(longitude, 200, 20);
    lv_obj_set_style_text_font(longitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(longitude, Longitude_formatString(GPS.location.lng()));
    lv_obj_set_pos(longitude, 45, 23);
    lv_obj_add_event_cb(longitude, update_longitude, LV_EVENT_VALUE_CHANGED, NULL);

    // Map Tile
    zoombox = lv_spinbox_create(map_tile);
    lv_spinbox_set_range(zoombox, MIN_ZOOM, MAX_ZOOM);
    lv_spinbox_set_digit_format(zoombox, 2, 0);
    lv_spinbox_set_value(zoombox, DEF_ZOOM);

    lv_obj_set_width(zoombox, 60);
    lv_obj_set_pos(zoombox, 2, 5);
    lv_group_focus_obj(zoombox);
    lv_obj_add_event_cb(zoombox, get_zoom_value, LV_EVENT_VALUE_CHANGED, NULL);

    sprArrow.createSprite(16, 16);
    sprArrow.setColorDepth(16);
    sprArrow.fillSprite(TFT_BLACK);
    sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
    lv_obj_add_event_cb(map_tile, draw_map, LV_EVENT_DRAW_MAIN_END, NULL);
    lv_obj_add_event_cb(map_tile, update_map, LV_EVENT_VALUE_CHANGED, NULL);

    // Satellite Tracking Tile
    pdop_label = lv_label_create(sat_track);
    lv_obj_set_size(pdop_label, 55, 40);
    lv_obj_set_style_text_font(pdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(pdop_label, "PDOP:\n%s", pdop.value());
    lv_obj_set_pos(pdop_label, 5, 5);

    hdop_label = lv_label_create(sat_track);
    lv_obj_set_size(hdop_label, 55, 40);
    lv_obj_set_style_text_font(hdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(hdop_label, "HDOP:\n%s", hdop.value());
    lv_obj_set_pos(hdop_label, 5, 40);

    vdop_label = lv_label_create(sat_track);
    lv_obj_set_size(vdop_label, 55, 40);
    lv_obj_set_style_text_font(vdop_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(vdop_label, "VDOP:\n%s", vdop.value());
    lv_obj_set_pos(vdop_label, 5, 75);

    alt_label = lv_label_create(sat_track);
    lv_obj_set_size(alt_label, 55, 80);
    lv_obj_set_style_text_font(alt_label, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(alt_label, "ALT:\n%4dm.", (int)GPS.altitude.meters());
    lv_obj_set_pos(alt_label, 5, 110);

    satbar_1 = lv_chart_create(sat_track);
    lv_obj_set_size(satbar_1, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satbar_1, 6, 0);
    lv_chart_set_range(satbar_1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser1 = lv_chart_add_series(satbar_1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_1, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_1, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satbar_1, 0, 160);

    satbar_2 = lv_chart_create(sat_track);
    lv_obj_set_size(satbar_2, TFT_WIDTH, 55);
    lv_chart_set_div_line_count(satbar_2, 6, 0);
    lv_chart_set_range(satbar_2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser2 = lv_chart_add_series(satbar_2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_2, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_2, (MAX_SATELLLITES_IN_VIEW / 2));
    lv_obj_set_pos(satbar_2, 0, 225);

    timer_main_scr = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(timer_main_scr);
}

/**
 * @brief Update Main Screen
 *
 */
void update_main_screen(lv_timer_t *t)
{
    int totalMessages = 0;
    int currentMessage = 0;
    switch (act_tile)
    {
    case COMPASS:
#ifdef ENABLE_COMPASS
        heading = read_compass();
        lv_event_send(compass_heading, LV_EVENT_VALUE_CHANGED, NULL);
#endif
        if (GPS.location.isUpdated())
        {
            lv_event_send(latitude, LV_EVENT_VALUE_CHANGED, NULL);
            lv_event_send(longitude, LV_EVENT_VALUE_CHANGED, NULL);
        }
        break;
    case MAP:
        if (GPS.location.isUpdated())
            lv_event_send(map_tile, LV_EVENT_VALUE_CHANGED, NULL);
        break;
    case SATTRACK:
        tft.startWrite();
        tft.drawCircle(165, 100, 60, TFT_WHITE);
        tft.drawCircle(165, 100, 30, TFT_WHITE);
        tft.drawCircle(165, 100, 1, TFT_WHITE);
        tft.setTextFont(2);
        tft.drawString("N", 162, 32);
        tft.drawString("S", 162, 152);
        tft.drawString("W", 102, 92);
        tft.drawString("E", 222, 92);
        tft.setTextFont(1);
        tft.endWrite();

        if (pdop.isUpdated() || hdop.isUpdated() || vdop.isUpdated())
        {
            lv_label_set_text_fmt(pdop_label, "PDOP:\n%s", pdop.value());
            lv_label_set_text_fmt(hdop_label, "HDOP:\n%s", hdop.value());
            lv_label_set_text_fmt(vdop_label, "VDOP:\n%s", vdop.value());
        }

        if (GPS.altitude.isUpdated())
            lv_label_set_text_fmt(alt_label, "ALT:\n%4dm.", (int)GPS.altitude.meters());

        if (totalGPGSVMessages.isUpdated())
        {
            for (int i = 0; i < 4; ++i)
            {
                int no = atoi(satNumber[i].value());
                if (no >= 1 && no <= MAX_SATELLITES)
                {
                    sat_tracker[no - 1].satnumber = atoi(satNumber[i].value());
                    sat_tracker[no - 1].elevation = atoi(elevation[i].value());
                    sat_tracker[no - 1].azimuth = atoi(azimuth[i].value());
                    sat_tracker[no - 1].snr = atoi(snr[i].value());
                    sat_tracker[no - 1].active = true;
                }
            }
        }

        totalMessages = atoi(totalGPGSVMessages.value());
        currentMessage = atoi(messageNumber.value());
        if (totalMessages == currentMessage)
        {
            for (int i = 0; i < (MAX_SATELLLITES_IN_VIEW / 2); i++)
            {
                satbar_ser1->y_points[i] = LV_CHART_POINT_NONE;
                satbar_ser2->y_points[i] = LV_CHART_POINT_NONE;
            }
            lv_chart_refresh(satbar_1);
            lv_chart_refresh(satbar_2);

            // ESFERA SATELITE TO-DO
            // SNR
            int active_sat = 0;
            for (int i = 0; i < MAX_SATELLLITES_IN_VIEW; ++i)
            {
                if (sat_tracker[i].active && (sat_tracker[i].snr > 0))
                {
                    lv_point_t p;
                    lv_area_t a;
                    if (active_sat < (MAX_SATELLLITES_IN_VIEW / 2))
                    {
                        satbar_ser1->y_points[active_sat] = sat_tracker[i].snr;
                        lv_chart_get_point_pos_by_id(satbar_1, satbar_ser1, active_sat, &p);

                        tft.setCursor(p.x - 2, (satbar_1->coords.y2) + 2);
                        tft.print(sat_tracker[i].satnumber);
                    }
                    else
                    {
                        satbar_ser2->y_points[active_sat - (MAX_SATELLLITES_IN_VIEW / 2)] = sat_tracker[i].snr;
                        lv_chart_get_point_pos_by_id(satbar_2, satbar_ser2, (active_sat - (MAX_SATELLLITES_IN_VIEW / 2)), &p);

                        tft.setCursor(p.x - 2, (satbar_2->coords.y2) + 2);
                        tft.print(sat_tracker[i].satnumber);
                    }
                    active_sat++;

                    int H = (60 * cos(DEGtoRAD(sat_tracker[i].elevation)));
                    int sat_pos_x = 165 + (H * sin(DEGtoRAD(sat_tracker[i].azimuth)));
                    int sat_pos_y = 100 - (H * cos(DEGtoRAD(sat_tracker[i].azimuth)));
                    sat_tracker[i].pos_x = sat_pos_x;
                    sat_tracker[i].pos_y = sat_pos_y;
                }
            }
        }
        lv_chart_refresh(satbar_1);
        lv_chart_refresh(satbar_2);
        break;
    default:
        break;
    }
}
