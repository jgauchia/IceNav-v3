/**
 * @file globalGuiDef.h
 * @brief  Global GUI Variables
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef GLOBALGUIDEF_HPP
#define GLOBALGUIDEF_HPP

#include <lvgl.h>
#include "tft.hpp"

/**
 * @brief Screens definitions
 *
 */
extern lv_obj_t *mainScreen;           // Main Screen
static lv_obj_t *tilesScreen;          // Tiles Screen
extern lv_obj_t *notifyBar;            // Notify Bar
extern lv_obj_t *settingsScreen;       // Settings Screen
extern lv_obj_t *mapSettingsScreen;    // Map Settings Screen
extern lv_obj_t *deviceSettingsScreen; // Device Settings Screen
extern bool needReboot;                // Flag to force device reboot

/**
 * @brief Get PNG width and height
 *
 * @param fs -> Filesystem
 * @param filename -> PNG file name
 * @param width -> PNG width
 * @param height -> PNG height
 * @return true/false if file doesn't exists
 */
static bool getPngSize(fs::FS &fs, const char* filename, uint16_t *width, uint16_t *height)
{
    File file = fs.open(filename);

    if (!file)
    {
        return false;
    }

    byte table[32];

    for (int i = 0; file.available() && i < 32; i++)
    {
        table[i] = file.read();
    }

    *width=table[16]*256*256*256+table[17]*256*256+table[18]*256+table[19];
    *height=table[20]*256*256*256+table[21]*256*256+table[22]*256+table[23];

    file.close();

    return true;
}

#endif


