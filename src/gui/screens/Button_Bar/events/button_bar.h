/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Button Bar events
 * @version 0.1.8
 * @date 2024-04
 */

static void loadOptions();

/**
 * @brief Flag for button activated
 *
 */
bool isWaypointOpt = false;
bool isTrackOpt = false;
bool isOptionLoaded = false;

/**
 * @brief Settings Button event
 *
 * @param event
 */
static void settings(lv_event_t *event)
{
    log_v("Settings");
    isMainScreen = false;
    lv_screen_load(settingsScreen);
}

/**
 * @brief Waypoint Button event
 *
 * @param event
 */
static void waypoint(lv_event_t *event)
{
    log_v("Waypoint");
    isMainScreen = false;
    isWaypointOpt = true;
    isTrackOpt = false;
    if (!isOptionLoaded)
    {
        isOptionLoaded = true;
        //loadOptions();
    }
}

/**
 * @brief Track Button event
 *
 * @param event
 */
static void track(lv_event_t *event)
{
    log_v("Track");
    isMainScreen = false;
    isTrackOpt = true;
    isWaypointOpt = false;
    if (!isOptionLoaded)
    {
        isOptionLoaded = true;
        //loadOptions();
    }
}