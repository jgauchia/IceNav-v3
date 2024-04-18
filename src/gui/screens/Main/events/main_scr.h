/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Main screen events
 * @version 0.1.8
 * @date 2024-04
 */

static void deleteMapScrSprites();
static void createMapScrSprites();

/**
 * @brief Flag to indicate when tileview was scrolled
 *
 */
bool isScrolled = true;

/**
 * @brief Flag to indicate when tileview scroll was finished
 *
 */
bool isReady = false;

/**
 * @brief Zoom sprite
 *
 */
TFT_eSprite zoomSprite = TFT_eSprite(&tft);

/**
 * @brief Active Tile in TileView control
 *
 */
uint8_t activeTile = 0;
enum tileName
{
    COMPASS,
    MAP,
    NAV,
    SATTRACK,
};

/**
 * @brief Get the active tile
 *
 * @param event
 */
static void getActTile(lv_event_t *event)
{
    if (isReady)
    {
        isScrolled = true;
        log_d("Free PSRAM: %d", ESP.getFreePsram());
        log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
        if (activeTile == MAP)
        {
            if (!isVectorMap)
                createMapScrSprites();
            refreshMap = true;
        }
    }
    else
    {
        isReady = true;
    }

    lv_obj_t *actTile = lv_tileview_get_tile_act(tiles);
    lv_coord_t tileX = lv_obj_get_x(actTile) / TFT_WIDTH;
    activeTile = tileX;
}

/**
 * @brief Tile start scrolling event
 *
 * @param event
 */
static void scrollTile(lv_event_t *event)
{
    isScrolled = false;
    isReady = false;

    if (!isVectorMap)
        deleteMapScrSprites();

    deleteSatInfoSprites();
}

/**
 * @brief Update Main Screen
 *
 */
static void updateMainScreen(lv_timer_t *t)
{
    if (isScrolled && isMainScreen)
    {
        switch (activeTile)
        {
        case COMPASS:
#ifdef ENABLE_COMPASS
            heading = getHeading();
            lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
#endif

            if (GPS.location.isUpdated())
            {
                lv_obj_send_event(latitude, LV_EVENT_VALUE_CHANGED, NULL);
               lv_obj_send_event(longitude, LV_EVENT_VALUE_CHANGED, NULL);
            }
            if (GPS.altitude.isUpdated())
            {
                lv_obj_send_event(altitude, LV_EVENT_VALUE_CHANGED, NULL);
            }

            if (GPS.speed.isUpdated())
                lv_obj_send_event(speedLabel, LV_EVENT_VALUE_CHANGED, NULL);

            break;

        case MAP:
            // if (GPS.location.isUpdated())
            lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
            break;

        case SATTRACK:
            lv_obj_send_event(satTrackTile, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        case NAV:
            break;
        default:
            break;
        }
    }
}
