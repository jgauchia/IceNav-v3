/**
 * @file globalGuiDef.h
 * @brief  Global GUI Variables
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <lvgl.h>
#include "tft.hpp"
#include "storage.hpp"

extern lv_display_t *display; /**< LVGL Display Driver */

/**
 * @brief Screens definitions
 *
 */
extern lv_obj_t *mainScreen;            /**< Main Screen */
extern lv_obj_t *tilesScreen;           /**< Tiles Screen */
extern lv_obj_t *notifyBarIcons;        /**< Notify Bar Icons */
extern lv_obj_t *notifyBarHour;         /**< Notify Bar Hour */
extern lv_obj_t *settingsScreen;        /**< Settings Screen */
extern lv_obj_t *mapSettingsScreen;     /**< Map Settings Screen */
extern lv_obj_t *deviceSettingsScreen;  /**< Device Settings Screen */
extern lv_obj_t *gpxDetailScreen;       /**< Add Waypoint Screen */
extern lv_obj_t *listGPXScreen;         /**< List Waypoint Screen */

extern lv_group_t *scrGroup;            /**< Screen group */
extern lv_group_t *keyGroup;            /**< GPIO group */

extern bool needReboot;                 /**< Flag to force device reboot */
extern bool isSearchingSat;             /**< Flag to indicate that device is searching for satellites */
extern lv_obj_t *buttonBar;             /**< Button Bar */
extern lv_obj_t *menuBtn;               /**< Button Menu */
extern lv_obj_t *gpxTagValue;           /**< Add/Edit Waypoint screen text area */
extern bool isScreenRotated;            /**< Flag to know if screen is rotated */
extern bool isTrackLoaded;              /**< Flag to know if track is loaded */

extern Storage storage;

#ifdef T4_S3
    static const lv_font_t *fontDefault     = &lv_font_montserrat_18;  /**< Default font for large screens */
    static const lv_font_t *fontSmall       = &lv_font_montserrat_14;  /**< Small font for large screens */
    static const lv_font_t *fontSatInfo     = &lv_font_montserrat_20;  /**< Satellite info font for large screens */
    static const lv_font_t *fontMedium      = &lv_font_montserrat_20;  /**< Medium font for large screens */
    static const lv_font_t *fontLarge       = &lv_font_montserrat_24;  /**< Large font for large screens */
    static const lv_font_t *fontLargeMedium = &lv_font_montserrat_28;  /**< Large-medium font for large screens */
    static const lv_font_t *fontVeryLarge   = &lv_font_montserrat_48;  /**< Very large font for large screens */
    static const lv_font_t *fontOptions     = &lv_font_montserrat_22;  /**< Options font for large screens */
    static const float scale                = 1.5f;                    /**< UI scale for large screens */ 
    static const float scaleBut             = 1.5f;                    /**< Button scale for large screens */
    static const float scaleSatInfo         = 1.5f;                    /**< Satellite info scale for large screens */
    static const float imgAlign             = 1.2f;                    /**< Image alignment scale for large screens */
#elif defined(LARGE_SCREEN)
    static const lv_font_t *fontDefault     = &lv_font_montserrat_14;  /**< Default font for large screens */
    static const lv_font_t *fontSmall       = &lv_font_montserrat_10;  /**< Small font for large screens */
    static const lv_font_t *fontSatInfo     = &lv_font_montserrat_16;  /**< Satellite info font for large screens */
    static const lv_font_t *fontMedium      = &lv_font_montserrat_16;  /**< Medium font for large screens */
    static const lv_font_t *fontLarge       = &lv_font_montserrat_20;  /**< Large font for large screens */
    static const lv_font_t *fontLargeMedium = &lv_font_montserrat_24;  /**< Large-medium font for large screens */
    static const lv_font_t *fontVeryLarge   = &lv_font_montserrat_48;  /**< Very large font for large screens */
    static const lv_font_t *fontOptions     = &lv_font_montserrat_18;  /**< Options font for large screens */
    static const float scale                = 1.0f;                    /**< UI scale for large screens */
    static const float scaleBut             = 1.0f;                    /**< Button scale for large screens */
    static const float scaleSatInfo         = 1.0f;                    /**< Satellite info scale for large screens */
#else
    static const lv_font_t *fontDefault     = &lv_font_montserrat_12;  /**< Default font for small screens */
    static const lv_font_t *fontSmall       = &lv_font_montserrat_8;   /**< Small font for small screens */
    static const lv_font_t *fontSatInfo     = &lv_font_montserrat_10;  /**< Satellite info font for small screens */
    static const lv_font_t *fontMedium      = &lv_font_montserrat_14;  /**< Medium font for small screens */
    static const lv_font_t *fontLarge       = &lv_font_montserrat_12;  /**< Large font for small screens */
    static const lv_font_t *fontLargeMedium = &lv_font_montserrat_16;  /**< Large-medium font for small screens */
    static const lv_font_t *fontVeryLarge   = &lv_font_montserrat_38;  /**< Very large font for small screens */
    static const lv_font_t *fontOptions     = &lv_font_montserrat_12;  /**< Options font for small screens */
    static const float scale                = 0.75f;                   /**< UI scale for small screens */
    static const float scaleBut             = 0.60f;                   /**< Button scale for small screens */
    static const float scaleSatInfo         = 0.80f;                   /**< Satellite info scale for small screens */
#endif


static const int iconScale   = LV_SCALE_NONE * scale;   /**< Icon scale factor */
static const int buttonScale = LV_SCALE_NONE * scaleBut; /**< Button scale factor */

/**
 * @brief Get PNG width and height from file.
 *
 * @param filename PNG file name.
 * @param width Pointer to store PNG width.
 * @param height Pointer to store PNG height.
 * @return true if the file exists and dimensions are read, false otherwise.
 */
static bool getPngSize(const char* filename, uint16_t *width, uint16_t *height)
{
    FILE* file = storage.open(filename, "r");

    if (!file)
        return false;

    uint8_t table[32];

    fread(table, sizeof(uint8_t), 32, file);

    *width=table[16]*256*256*256+table[17]*256*256+table[18]*256+table[19];
    *height=table[20]*256*256*256+table[21]*256*256+table[22]*256+table[23];

    storage.close(file);

    return true;
}


