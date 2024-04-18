/**
 * @file options.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Waypoint, track events
 * @version 0.1.8
 * @date 2024-04
 */

// /**
//  * @brief Options box close event
//  *
//  * @param event
//  */
// static void close_option(lv_event_t *event)
// {
//     isMainScreen = true;
//     isOptionLoaded = false;
//     refreshMap=true;
// }

/**
 * @brief Save Option
 *
 * @param event
 */
static void saveOption(lv_event_t *event)
{
    log_v("Save Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Load Option
 *
 * @param event
 */
static void loadOption(lv_event_t *event)
{
    log_v("Load Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}

/**
 * @brief Delete Option
 *
 * @param event
 */
static void deleteOption(lv_event_t *event)
{
    log_v("Delete Option");
    isMainScreen = true;
    isOptionLoaded = false;
    lv_msgbox_close(option);
}