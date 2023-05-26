/**
 * @file main_screen.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Main screen events
 * @version 0.1.4
 * @date 2023-05-23
 */

/**
 * @brief Flag to indicate when maps needs to be draw
 *
 */
bool is_map_draw = false;

/**
 * @brief Flag to indicate when tile map is found on SD
 *
 */
bool map_found = false;

/**
 * @brief Flag to indicate when tileview was scrolled
 *
 */
bool is_scrolled = true;

/**
 * @brief Flag to indicate when tileview scroll was finished
 *
 */
bool is_ready = false;

/**
 * @brief Old Map tile coordinates and zoom
 *
 */
MapTile OldMapTile = {"", 0, 0, 0};

/**
 * @brief Zoom Levels and Default zoom
 *
 */
#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
uint8_t zoom = DEF_ZOOM;

/**
 * @brief Active Tile in TileView control
 *
 */
uint8_t act_tile = 0;
enum tilename
{
    COMPASS,
    MAP,
    SATTRACK,
};

/**
 * @brief Get the active tile
 *
 * @param event
 */
static void get_act_tile(lv_event_t *event)
{
    if (is_ready)
    {
        lv_timer_resume(timer_main);
        is_scrolled = true;
    }
    else
        is_ready = true;

    lv_obj_t *acttile = lv_tileview_get_tile_act(tiles);
    lv_coord_t tile_x = lv_obj_get_x(acttile) / TFT_WIDTH;
    act_tile = tile_x;
    // if (act_tile == MAP)
    // {
    //     if (is_ready)
    //     {
    //         lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
    //         lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
    //     }
    // }
}

/**
 * @brief Tile start scrolling event
 *
 * @param event
 */
static void scroll_tile(lv_event_t *event)
{
    lv_timer_pause(timer_main);
    is_scrolled = false;
    is_ready = false;
}

/**
 * @brief Update Main Screen
 *
 */
void update_main_screen(lv_timer_t *t)
{
    if (is_scrolled)
    {
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
            if (GPS.altitude.isUpdated())
            {
                lv_event_send(altitude, LV_EVENT_VALUE_CHANGED, NULL);
            }
            break;

        case MAP:
            // if (GPS.location.isUpdated())
            lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
            break;

        case SATTRACK:
            lv_event_send(sat_track_tile, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        default:
            break;
        }
    }
}
