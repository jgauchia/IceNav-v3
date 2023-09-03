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
 * @brief Global Variables definition for device preferences & config.
 * 
 */
float offx = 0.0, offy = 0.0; // Compass offset calibration
bool map_rotation = true;     // Map Compass Rotation


/**
 * @brief Load stored preferences
 * 
 */
static void load_preferences()
{
    preferences.begin("ICENAV",false);
    offx = preferences.getFloat("C_offset_x",0.0);
    offy = preferences.getFloat("C_offset_y",0.0);
    map_rotation = preferences.getBool("Map_rot",false);
    
    log_v("OFFSET X  %f",offx);
    log_v("OFFSET Y  %f",offy);
    log_v("MAP ROTATION %d",map_rotation);

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
    preferences.begin("ICENAV",false);
    preferences.putFloat("C_offset_x",offset_x);
    preferences.putFloat("C_offset_y",offset_y);
    preferences.end();
}