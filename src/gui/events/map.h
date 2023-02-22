/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1
 * @date 2022-11-20
 */

/**
 * @brief Zoom Levels and Default zoom
 *
 */
#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
int zoom = DEF_ZOOM;

/**
 * @brief Old tile coordinates and zoom
 *
 */
// int tilex_old = 0;
// int tiley_old = 0;
// int zoom_old = 0;

MapTile CurrentMapTile;
MapTile OldMapTile;

/**
 * @brief Navitagion Arrow position on screen
 *
 */
ScreenCoord NavArrow_position;

/**
 * @brief Flag to indicate when tile map is found on SD
 *
 */
bool map_found = false;

/**
 * @brief Sprite for Navigation Arrow in map tile
 *
 */
TFT_eSprite sprArrow = TFT_eSprite(&tft);

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
    zoom = (int)lv_slider_get_value(zoom_slider);
    lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
    lv_event_send(map_tile, LV_EVENT_VALUE_CHANGED, NULL);
}

/**
 * @brief Draw map event
 *
 * @param event
 */
static void draw_map(lv_event_t *event)
{
    if (!is_map_draw)
    {
        OldMapTile.zoom = 0;
        OldMapTile.tilex = 0;
        OldMapTile.tiley = 0;
        OldMapTile.file = "";  // TODO - Delete Warning 
        is_map_draw = true;
        map_found = false;
    }
}

/**
 * @brief Update map event
 *
 * @param event
 */
static void update_map(lv_event_t *event)
{
    CurrentMapTile = get_map_tile(GPS.location.lng(), GPS.location.lat(), zoom);
    if (strcmp(CurrentMapTile.file, OldMapTile.file) != 0 || CurrentMapTile.zoom != OldMapTile.zoom)
    {
        // if (CurrentMapTile.zoom != zoom_old || (CurrentMapTile.tiley != tilex_old || CurrentMapTile.tiley != tiley_old))
        {
            OldMapTile.zoom = CurrentMapTile.zoom;
            OldMapTile.tilex = CurrentMapTile.tilex;
            OldMapTile.tiley = CurrentMapTile.tiley;
            OldMapTile.file = CurrentMapTile.file;
            tft.startWrite();
            map_found = tft.drawPngFile(SD, CurrentMapTile.file, 32, 64);
            // map_found = tft.drawPngFile(SD, "/MAP/6/32/23.png", 32, 64);
            tft.endWrite();
        }
    }
    if (map_found)
    {
        sprArrow.deleteSprite();
        sprArrow.createSprite(16, 16);
        // sprArrow.setColorDepth(16);
        sprArrow.fillSprite(TFT_BLACK);
        // sprArrow.drawCircle(8,8,5,TFT_RED);
        sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);

        NavArrow_position = coord_to_scr_pos(32, 64, GPS.location.lng(), GPS.location.lat(), zoom);
#ifdef ENABLE_COMPASS
        heading = read_compass();
        tft.startWrite();
        // sprArrow.drawCircle(8, 8, 5, TFT_RED);
        // tft.drawPngFile(SD, CurrentMapTile.file, NavArrow.posx, NavArrow.posy, 16, 16, NavArrow.posx, NavArrow.posy);
        tft.setPivot(NavArrow_position.posx, NavArrow_position.posy);
        sprArrow.pushRotated(heading, TFT_BLACK);
        // sprArrow.pushRotated(heading, TFT_TRANSPARENT);
        //  sprArrow.pushSprite(NavArrow.posx, NavArrow.posy, TFT_BLACK);
        tft.endWrite();
#else
        tft.startWrite();
        sprArrow.pushSprite(NavArrow.posx, NavArrow.posy, TFT_BLACK);
        tft.endWrite();
#endif
    }
}