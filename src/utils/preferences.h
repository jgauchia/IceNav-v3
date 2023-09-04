/**
 * @file preferences.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Preferences functions
 * @version 0.1.6
 * @date 2023-06-14
 */

#include <Preferences.h>

Preferences preferences;

/**
 * @brief Zoom Levels and Default zoom
 *
 */
#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
uint8_t zoom = 0;

/**
 * @brief Global Variables definition for device preferences & config.
 *
 */
float offx = 0.0, offy = 0.0; // Compass offset calibration
bool map_rotation = true;     // Map Compass Rotation
uint8_t def_zoom = 0;         // Default Zoom Value
bool show_map_compass = true; // Compass in map screen

/**
 * @brief Load stored preferences
 *
 */
static void load_preferences()
{
    preferences.begin("ICENAV", false);
    offx = preferences.getFloat("C_offset_x", 0.0);
    offy = preferences.getFloat("C_offset_y", 0.0);
    map_rotation = preferences.getBool("Map_rot", false);
    def_zoom = preferences.getUInt("Def_zoom", DEF_ZOOM);
    zoom = def_zoom;
    show_map_compass = preferences.getBool("Map_compass",true);

    log_v("COMPASS OFFSET X  %f", offx);
    log_v("COMPASS OFFSET Y  %f", offy);
    log_v("MAP ROTATION %d", map_rotation);
    log_v("DEFAULT ZOOM LEVEL %d", zoom);
    log_v("SHOW MAP COMPASS %d",show_map_compass);

    preferences.end();
}

/**
 * @brief Save Map Rotation Type
 * 
 * @param zoom_rotation 
 */
static void save_map_rotation(bool zoom_rotation)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_rot", zoom_rotation);
    preferences.end();
}

/**
 * @brief Save current compass calibration in preferences
 *
 * @param offset_x
 * @param offset_y
 */
static void save_compass_cal(float offset_x, float offset_y)
{
    preferences.begin("ICENAV", false);
    preferences.putFloat("C_offset_x", offset_x);
    preferences.putFloat("C_offset_y", offset_y);
    preferences.end();
}

/**
 * @brief Save default zoom value
 *
 * @param default_zoom
 */
static void save_default_zoom(uint8_t default_zoom)
{
    preferences.begin("ICENAV", false);
    preferences.putUInt("Def_zoom", def_zoom);
    preferences.end();
}