/**
 * @file globalGuiDef.h
 * @brief  Global GUI Variables
 * @version 0.1.8
 * @date 2024-05
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

#endif
