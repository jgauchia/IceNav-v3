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
bool show_map_speed = true;   // Speed in map screen
bool show_map_scale = true;   // Scale in map screen
// uint16_t gps_speed = 0;       // GPS Speed (see gps.h)
// uint16_t gps_update = 0;      // GPS Update rate (see gps.h)
int compass_pos_x = 0;
int compass_pos_y = 0;

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
    show_map_compass = preferences.getBool("Map_compass", false);
    show_map_speed = preferences.getBool("Map_speed", false);
    show_map_scale = preferences.getBool("Map_scale", false);
    gps_speed = preferences.getShort("GPS_speed", 2);
    gps_update = preferences.getShort("GPS_rate", 3);
    compass_pos_x = preferences.getInt("Compass_X", (TFT_WIDTH / 2) - 100);
    compass_pos_y = preferences.getInt("Compass_Y", (TFT_HEIGHT / 2) - 60);

    log_v("COMPASS OFFSET X  %f", offx);
    log_v("COMPASS OFFSET Y  %f", offy);
    log_v("MAP ROTATION %d", map_rotation);
    log_v("DEFAULT ZOOM LEVEL %d", zoom);
    log_v("SHOW MAP COMPASS %d", show_map_compass);
    log_v("SHOW MAP SPEED %d", show_map_speed);
    log_v("SHOW MAP SCALE %d", show_map_scale);
    log_v("GPS SPEED %d", gps_speed);
    log_v("GPS UPDATE RATE %d", gps_update);
    log_v("COMPASS POS X %d", compass_pos_x);
    log_v("COMPASS POS Y %d", compass_pos_y);

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

/**
 * @brief Save show compass in map
 *
 * @param show_compass
 */
static void save_show_compass(bool show_compass)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_compass", show_compass);
    preferences.end();
}

/**
 * @brief Save show speed in map
 *
 * @param show_speed
 */
static void save_show_speed(bool show_speed)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_speed", show_speed);
    preferences.end();
}

/**
 * @brief Save show scale in map
 *
 * @param show_scale
 */
static void save_show_scale(bool show_scale)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_scale", show_scale);
    preferences.end();
}

/**
 * @brief Save GPS speed
 *
 * @param gps_speed
 */
static void save_gps_speed(uint16_t gps_speed)
{
    preferences.begin("ICENAV", false);
    preferences.putShort("GPS_speed", gps_speed);
    preferences.end();
#ifdef AT6558D_GPS
    gps->flush();
    gps->println(GPS_BAUD_PCAS[gps_speed]);
    gps->flush();
    gps->println("$PCAS00*01\r\n");
    gps->flush();
    delay(500);
#endif
    gps->flush();
    gps->end();
    delay(500);
    gps->begin(GPS_BAUD[gps_speed], SERIAL_8N1, GPS_RX, GPS_TX);
    delay(500);
}

/**
 * @brief Save GPS Update rate
 *
 * @param gps_update_rate
 */
static void save_gps_update_rate(uint16_t gps_update_rate)
{
    preferences.begin("ICENAV", false);
    preferences.putShort("GPS_rate", gps_update_rate);
    preferences.end();
#ifdef AT6558D_GPS
    gps->flush();
    gps->println(GPS_RATE_PCAS[gps_update_rate]);
    gps->flush();
    gps->println("$PCAS00*01\r\n");
    gps->flush();
    delay(500);
#endif
}

/**
 * @brief Save Compass Widget position
 *
 * @param pos_x
 * @param pos_y
 */
static void save_compass_pos(int pos_x, int pos_y)
{
    preferences.begin("ICENAV", false);
    preferences.putInt("Compass_X", pos_x);
    preferences.putInt("Compass_Y", pos_y);
    preferences.end();
}