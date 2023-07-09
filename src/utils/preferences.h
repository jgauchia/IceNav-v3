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
 * @brief Load stored preferences
 * 
 */
static void load_preferences()
{
    preferences.begin("ICENAV",false);
    offx = preferences.getInt("Compass_offset_x",0);
    offy = preferences.getInt("Compass_offset_y",0);
    preferences.end();
}