/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-16
 */
int heading = 0;
int last_heading = 0;

#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
int zoom = DEF_ZOOM;
MapTile CurrentMapTile;
int tilex_old = 0;
int tiley_old = 0;
int zoom_old = 0;
ScreenCoord NavArrow;
bool map_found = false;

#define UPDATE_MAINSCR_PERIOD 10
void update_main_screen(lv_timer_t *t);

static lv_obj_t *compass_heading;
static lv_obj_t *compass_img;
static lv_obj_t *latitude;
static lv_obj_t *longitude;
static lv_obj_t *zoom_label;
static lv_obj_t *pdop_label;
static lv_obj_t *hdop_label;
static lv_obj_t *vdop_label;
static lv_obj_t *alt_label;
lv_obj_t *satbar_1;
lv_obj_t *satbar_2;
lv_chart_series_t *satbar_ser1;
lv_chart_series_t *satbar_ser2;

static lv_timer_t *timer_main_scr;
static lv_obj_t *zoombox;
static lv_style_t style_zoom;
TFT_eSprite sprArrow = TFT_eSprite(&tft);

/**
 * @brief Draw map event
 *
 * @param event
 */
static void drawmap(lv_event_t *event)
{
    if (!is_map_draw && act_tile == MAP)
    {
        CurrentMapTile = get_map_tile(GPS.location.lng(), GPS.location.lat(), zoom);
        if (CurrentMapTile.zoom != zoom_old || (CurrentMapTile.tiley != tilex_old || CurrentMapTile.tiley != tiley_old))
        {
            map_found = draw_png(SD, CurrentMapTile.file, 0, 64);
            is_map_draw = true;
            zoom_old = CurrentMapTile.zoom;
            tilex_old = CurrentMapTile.tilex;
            tiley_old = CurrentMapTile.tiley;
        }
        if (map_found)
        {
            NavArrow = coord_to_scr_pos(0, 64, GPS.location.lng(), GPS.location.lat(), zoom);
            tft.startWrite();
            sprArrow.pushSprite(NavArrow.posx, NavArrow.posy, TFT_BLACK);
            tft.endWrite();
        }
    }
}

/**
 * @brief Get the zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
    zoom = lv_spinbox_get_value(zoombox);
}

/**
 * @brief Create a main screen
 *
 */
void create_main_scr()
{
    mainScreen = lv_obj_create(NULL);

    // Main Screen Tiles
    tiles = lv_tileview_create(mainScreen);
    lv_obj_t *compass = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *map = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *sat_track = lv_tileview_add_tile(tiles, 2, 0, LV_DIR_LEFT);
    lv_obj_set_size(tiles, 240, 300);
    lv_obj_set_pos(tiles, 0, 20);

    // Compass Tile
#ifdef ENABLE_COMPASS
    compass_heading = lv_label_create(compass);
    lv_obj_set_size(compass_heading, 150, 48);
    lv_obj_set_align(compass_heading, LV_ALIGN_CENTER);
    lv_obj_set_y(compass_heading, 35);
    lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);

    LV_IMG_DECLARE(arrow);
    lv_obj_t *arrow_img = lv_img_create(compass);
    lv_img_set_src(arrow_img, &arrow);
    lv_obj_align(arrow_img, LV_ALIGN_CENTER, 0, -20);

    LV_IMG_DECLARE(bruj);
    compass_img = lv_img_create(compass);
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

    longitude = lv_label_create(tiles);
    lv_obj_set_size(longitude, 200, 20);
    lv_obj_set_style_text_font(longitude, &lv_font_montserrat_16, 0);
    lv_label_set_text(longitude, Longitude_formatString(GPS.location.lng()));
    lv_obj_set_pos(longitude, 45, 23);

    // Map Tile
    zoombox = lv_spinbox_create(map);
    lv_spinbox_set_range(zoombox, MIN_ZOOM, MAX_ZOOM);
    lv_spinbox_set_digit_format(zoombox, 2, 0);
    lv_spinbox_set_value(zoombox, DEF_ZOOM);

    lv_style_init(&style_zoom);
    lv_style_set_outline_color(&style_zoom, lv_color_black());
    lv_style_set_outline_opa(&style_zoom, LV_OPA_20);
    lv_style_set_outline_width(&style_zoom, 2);
    lv_style_set_text_align(&style_zoom, LV_TEXT_ALIGN_RIGHT);
    lv_obj_add_style(zoombox, &style_zoom, LV_STATE_FOCUSED);

    LV_IMG_DECLARE(magnify);
    lv_obj_t *zoom_img = lv_img_create(map);
    lv_img_set_src(zoom_img, &magnify);
    lv_obj_set_pos(zoom_img, 10, 13);

    lv_obj_set_width(zoombox, 60);
    lv_obj_set_pos(zoombox, 2, 5);
    lv_group_focus_obj(zoombox);
    lv_obj_add_event_cb(zoombox, get_zoom_value, LV_EVENT_VALUE_CHANGED, NULL);

    sprArrow.createSprite(16, 16);
    sprArrow.setColorDepth(16);
    sprArrow.fillSprite(TFT_BLACK);
    sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);

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
    lv_obj_set_size(satbar_1, 240, 60);
    lv_obj_set_pos(satbar_1, 0, 160);
    lv_chart_set_div_line_count(satbar_1, 6, 0);
    lv_chart_set_range(satbar_1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser1 = lv_chart_add_series(satbar_1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_1, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_1, 12);

    satbar_2 = lv_chart_create(sat_track);
    lv_obj_set_size(satbar_2, 240, 60);
    lv_obj_set_pos(satbar_2, 0, 220);
    lv_chart_set_div_line_count(satbar_2, 6, 0);
    lv_chart_set_range(satbar_2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satbar_ser2 = lv_chart_add_series(satbar_2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satbar_2, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(satbar_2, 12);

    timer_main_scr = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(timer_main_scr);

    lv_obj_add_event_cb(map, drawmap, LV_EVENT_DRAW_MAIN_END, NULL);
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
        if (show_degree)
        {
            lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);
            lv_label_set_text_fmt(compass_heading, "%5d\xC2\xB0", heading);
        }
        else
        {
            if (GPS.altitude.isUpdated())
            {
                lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_38, 0);
                lv_label_set_text_fmt(compass_heading, " %4dm", (int)GPS.altitude.meters());
            }
        }
        lv_img_set_angle(compass_img, -(heading * 10));
#endif
        if (GPS.location.isUpdated())
        {
            lv_label_set_text(latitude, Latitude_formatString(GPS.location.lat()));
            lv_label_set_text(longitude, Longitude_formatString(GPS.location.lng()));
        }
        break;
    case MAP:
        if (GPS.location.isUpdated())
        {
            CurrentMapTile = get_map_tile(GPS.location.lng(), GPS.location.lat(), zoom);
            if (CurrentMapTile.zoom != zoom_old || (CurrentMapTile.tiley != tilex_old || CurrentMapTile.tiley != tiley_old))
            {
                zoom_old = CurrentMapTile.zoom;
                tilex_old = CurrentMapTile.tilex;
                tiley_old = CurrentMapTile.tiley;
                map_found = draw_png(SD, CurrentMapTile.file, 0, 64);
            }
            if (map_found)
            {
                NavArrow = coord_to_scr_pos(0, 64, GPS.location.lng(), GPS.location.lat(), zoom);
#ifdef ENABLE_COMPASS
                heading = read_compass();
                tft.startWrite();
                tft.setPivot(NavArrow.posx, NavArrow.posy);
                sprArrow.pushRotated(heading, TFT_BLACK);
                tft.endWrite();
#else
                tft.startWrite();
                sprArrow.pushSprite(NavArrow.posx, NavArrow.posy, TFT_BLACK);
                tft.endWrite();
#endif
            }
        }
        break;
    case SATTRACK:

        tft.startWrite();
        tft.drawCircle(165, 100, 60, TFT_WHITE);
        tft.drawCircle(165, 100, 30, TFT_WHITE);
        tft.drawCircle(165, 100, 1, TFT_WHITE);
        tft.drawString("N", 162, 32, 2);
        tft.drawString("S", 162, 152, 2);
        tft.drawString("W", 102, 92, 2);
        tft.drawString("E", 222, 92, 2);
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
            // ESFERA SATELITE TO-DO
            // SNR
            int active_sat = 0;
            for (int i = 0; i < MAX_SATELLITES; ++i)
            {
                if (sat_tracker[i].active)
                {
                    if (active_sat < 13)
                    {
                        satbar_ser1->y_points[active_sat] = sat_tracker[i].snr;
                    }
                    else
                    {
                        satbar_ser2->y_points[active_sat] = sat_tracker[i].snr;
                    }
                    active_sat++;
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
