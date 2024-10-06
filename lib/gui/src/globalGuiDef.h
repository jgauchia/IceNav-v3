/**
 * @file globalGuiDef.h
 * @brief  Global GUI Variables
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef GLOBALGUIDEF_H
#define GLOBALGUIDEF_H

#include <lvgl.h>
#include "tft.hpp"

/**
 * @Brief LVGL Display Driver
 * 
 */
extern lv_display_t *display;

/**
 * @brief Screens definitions
 *
 */
extern lv_obj_t *mainScreen;           // Main Screen
extern lv_obj_t *tilesScreen;          // Tiles Screen
extern lv_obj_t *notifyBarIcons;       // Notify Bar Icons
extern lv_obj_t *notifyBarHour;        // Notify Bar Hour
extern lv_obj_t *settingsScreen;       // Settings Screen
extern lv_obj_t *mapSettingsScreen;    // Map Settings Screen
extern lv_obj_t *deviceSettingsScreen; // Device Settings Screen
extern lv_obj_t *waypointScreen;       // Add Waypoint Screen
extern lv_obj_t *listWaypointScreen;   // List Waypoint Screen

extern lv_group_t * scrGroup;          // Screen group
extern lv_group_t * keyGroup;          // GPIO group

extern bool needReboot;                // Flag to force device reboot
extern bool isSearchingSat;            // Flag to indicate that is searching satellites
extern lv_obj_t *buttonBar;            // Button Bar
extern lv_obj_t *menuBtn;              // Button Menu
extern lv_obj_t *waypointName;         // Add / Edit Waypoint screen text area
extern bool isScreenRotated;           // Flag to know if screen is rotated

#ifdef LARGE_SCREEN
  static const lv_font_t *fontDefault = &lv_font_montserrat_14;
  static const lv_font_t *fontSmall = &lv_font_montserrat_10;
  static const lv_font_t *fontSatInfo = &lv_font_montserrat_16;
  static const lv_font_t *fontMedium = &lv_font_montserrat_16;
  static const lv_font_t *fontLarge = &lv_font_montserrat_20;
  static const lv_font_t *fontLargeMedium = &lv_font_montserrat_24;
  static const lv_font_t *fontVeryLarge = &lv_font_montserrat_48;
  static const lv_font_t *fontOptions = &lv_font_montserrat_18;
  static const float scale = 1.0f;
  static const float scaleBut = 1.0f;
  static const float scaleSatInfo = 1.0f;
#else
  static const lv_font_t *fontDefault = &lv_font_montserrat_12;
  static const lv_font_t *fontSmall = &lv_font_montserrat_8;
  static const lv_font_t *fontSatInfo = &lv_font_montserrat_10;
  static const lv_font_t *fontMedium = &lv_font_montserrat_14;
  static const lv_font_t *fontLarge = &lv_font_montserrat_12;
  static const lv_font_t *fontLargeMedium = &lv_font_montserrat_16;
  static const lv_font_t *fontVeryLarge = &lv_font_montserrat_38;
  static const lv_font_t *fontOptions = &lv_font_montserrat_12;
  static const float scale = 0.75f;
  static const float scaleBut = 0.60f;
  static const float scaleSatInfo = 0.80f;
#endif

static const int iconScale = LV_SCALE_NONE * scale;
static const int buttonScale = LV_SCALE_NONE * scaleBut;

/**
 * @brief Get PNG width and height
 *
 * @param filename -> PNG file name
 * @param width -> PNG width
 * @param height -> PNG height
 * @return true/false if file doesn't exists
 */
static bool getPngSize(const char* filename, uint16_t *width, uint16_t *height)
{
  FILE* file = fopen(filename, "r");

  if (!file)
  {
    return false;
  }

  byte table[32];

  // for (int i = 0; file.available() && i < 32; i++)
  // {
  //   table[i] = file.read();
  // }

  fread(table, sizeof(byte), 32, file);

  *width=table[16]*256*256*256+table[17]*256*256+table[18]*256+table[19];
  *height=table[20]*256*256*256+table[21]*256*256+table[22]*256+table[23];

  fclose(file);

  return true;
}

#endif


