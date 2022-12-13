/**
 * @file map_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1
 * @date 2022-11-20
 */

#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
int zoom = DEF_ZOOM;
int tilex_old = 0;
int tiley_old = 0;
int zoom_old = 0;
MapTile CurrentMapTile;
MapTile OldMapTile;
ScreenCoord NavArrow;
bool map_found = false;

/**
 * @brief Flag to indicate when maps needs to be draw
 *
 */
bool is_map_draw = false;

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
    zoom = (int)lv_slider_get_value(zoom_slider);
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
        OldMapTile.file = "";
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
            map_found = tft.drawPngFile(SD, CurrentMapTile.file, 0, 64);
            //debug->println(CurrentMapTile.file);
        }
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