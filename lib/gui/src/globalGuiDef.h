/**
 * @file globalGuiDef.h
 * @brief  Global GUI Variables
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <lvgl.h>
#include "tft.hpp"
#include "storage.hpp"
#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern lv_display_t *display_drv; /**< LVGL Display Driver */

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
extern lv_obj_t *sensorScreen;          /**< Sensor Info Screen */
extern lv_obj_t *gpxDetailScreen;       /**< Add Waypoint Screen */
extern lv_obj_t *listGPXScreen;         /**< List Waypoint Screen */

extern lv_group_t *scrGroup;            /**< Screen group */
extern lv_group_t *keyGroup;            /**< GPIO group */

extern bool needReboot;                 /**< Flag to force device reboot */
extern bool isSearchingSat;             /**< Flag to indicate that device is searching for satellites */
extern lv_obj_t *optionsPanel;          /**< Options Panel */
extern lv_obj_t *menuBtn;               /**< Button Menu */
extern lv_obj_t *optionsScrim;          /**< Bottom sheet dim scrim */
extern lv_obj_t *gpxTagValue;           /**< Add/Edit Waypoint screen text area */
extern bool isScreenRotated;            /**< Flag to know if screen is rotated */
extern bool isTrackLoaded;              /**< Flag to know if track is loaded */
extern float             routeDstLat;   /**< Router destination latitude */
extern float             routeDstLon;   /**< Router destination longitude */
extern std::atomic<bool> rerouteRequested; /**< Flag to trigger A* route calculation */
extern SemaphoreHandle_t routeMutex;    /**< Mutex protecting trackData during route updates */

extern Storage storage;

#ifdef T4_S3
    inline constexpr const lv_font_t *fontDefault     = &lv_font_montserrat_18;  /**< Default font for large screens */
    inline constexpr const lv_font_t *fontSmall       = &lv_font_montserrat_14;  /**< Small font for large screens */
    inline constexpr const lv_font_t *fontSatInfo     = &lv_font_montserrat_20;  /**< Satellite info font for large screens */
    inline constexpr const lv_font_t *fontMedium      = &lv_font_montserrat_20;  /**< Medium font for large screens */
    inline constexpr const lv_font_t *fontLarge       = &lv_font_montserrat_24;  /**< Large font for large screens */
    inline constexpr const lv_font_t *fontLargeMedium = &lv_font_montserrat_28;  /**< Large-medium font for large screens */
    inline constexpr const lv_font_t *fontVeryLarge   = &lv_font_montserrat_48;  /**< Very large font for large screens */
    inline constexpr const lv_font_t *fontOptions     = &lv_font_montserrat_22;  /**< Options font for large screens */
    inline constexpr float scale                      = 1.5f;                    /**< UI scale for large screens */
    inline constexpr float scaleBut                   = 1.5f;                    /**< Button scale for large screens */
    inline constexpr float scaleSatInfo               = 1.5f;                    /**< Satellite info scale for large screens */
    inline constexpr float imgAlign                   = 1.2f;                    /**< Image alignment scale for large screens */
#elif defined(LARGE_SCREEN)
    inline constexpr const lv_font_t *fontDefault     = &lv_font_montserrat_14;  /**< Default font for large screens */
    inline constexpr const lv_font_t *fontSmall       = &lv_font_montserrat_10;  /**< Small font for large screens */
    inline constexpr const lv_font_t *fontSatInfo     = &lv_font_montserrat_16;  /**< Satellite info font for large screens */
    inline constexpr const lv_font_t *fontMedium      = &lv_font_montserrat_16;  /**< Medium font for large screens */
    inline constexpr const lv_font_t *fontLarge       = &lv_font_montserrat_20;  /**< Large font for large screens */
    inline constexpr const lv_font_t *fontLargeMedium = &lv_font_montserrat_24;  /**< Large-medium font for large screens */
    inline constexpr const lv_font_t *fontVeryLarge   = &lv_font_montserrat_48;  /**< Very large font for large screens */
    inline constexpr const lv_font_t *fontOptions     = &lv_font_montserrat_18;  /**< Options font for large screens */
    inline constexpr float scale                      = 1.0f;                    /**< UI scale for large screens */
    inline constexpr float scaleBut                   = 1.0f;                    /**< Button scale for large screens */
    inline constexpr float scaleSatInfo               = 1.0f;                    /**< Satellite info scale for large screens */
#else
    inline constexpr const lv_font_t *fontDefault     = &lv_font_montserrat_12;  /**< Default font for small screens */
    inline constexpr const lv_font_t *fontSmall       = &lv_font_montserrat_8;   /**< Small font for small screens */
    inline constexpr const lv_font_t *fontSatInfo     = &lv_font_montserrat_10;  /**< Satellite info font for small screens */
    inline constexpr const lv_font_t *fontMedium      = &lv_font_montserrat_14;  /**< Medium font for small screens */
    inline constexpr const lv_font_t *fontLarge       = &lv_font_montserrat_12;  /**< Large font for small screens */
    inline constexpr const lv_font_t *fontLargeMedium = &lv_font_montserrat_16;  /**< Large-medium font for small screens */
    inline constexpr const lv_font_t *fontVeryLarge   = &lv_font_montserrat_38;  /**< Very large font for small screens */
    inline constexpr const lv_font_t *fontOptions     = &lv_font_montserrat_12;  /**< Options font for small screens */
    inline constexpr float scale                      = 0.75f;                   /**< UI scale for small screens */
    inline constexpr float scaleBut                   = 0.60f;                   /**< Button scale for small screens */
    inline constexpr float scaleSatInfo               = 0.80f;                   /**< Satellite info scale for small screens */
#endif


inline constexpr int iconScale   = LV_SCALE_NONE * scale;    /**< Icon scale factor */
inline constexpr int buttonScale = LV_SCALE_NONE * scaleBut;  /**< Button scale factor */


bool getPngSize(const char* filename, uint16_t *width, uint16_t *height);

