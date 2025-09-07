/**
 * @file settings.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Settings functions
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <EasyPreferences.hpp>
#include <NMEAGPS.h>
#include "globalGpxDef.h"
#include "tft.hpp"
#include "gps.hpp"
#include "battery.hpp"
#include "compass.hpp"

/**
 * @brief Map and Zoom configuration variables
 *
 */
extern uint8_t minZoom;         /**< Minimum Zoom Level */
extern uint8_t maxZoom;         /**< Maximum Zoom Level */
extern uint8_t defZoomRender;   /**< Default Zoom Level for rendering map */
extern uint8_t defZoomVector;   /**< Default Zoom Level for vector map */
extern uint8_t zoom;            /**< Current Zoom Level */
extern uint8_t defBright;       /**< Default screen brightness */
extern uint8_t defaultZoom;     /**< Default Zoom Value */

/** 
 * @brief UI and toolbar settings
 *
 */
extern bool showMapToolBar;     /**< Show Map Toolbar */

/** 
 *@brief GPS and sensor settings
 *
 */
extern uint16_t gpsBaud;        /**< GPS Baud Rate */
extern uint16_t gpsUpdate;      /**< GPS Update rate (Hz or ms) */
extern uint16_t compassPosX;    /**< Compass widget position X */
extern uint16_t compassPosY;    /**< Compass widget position Y */
extern uint16_t coordPosX;      /**< Coordinates widget position X */
extern uint16_t coordPosY;      /**< Coordinates widget position Y */
extern uint16_t altitudePosX;   /**< Altitude widget position X */
extern uint16_t altitudePosY;   /**< Altitude widget position Y */
extern uint16_t speedPosX;      /**< Speed widget position X */
extern uint16_t speedPosY;      /**< Speed widget position Y */
extern uint16_t sunPosX;        /**< Sunrise/Sunset widget position X */
extern uint16_t sunPosY;        /**< Sunrise/Sunset widget position Y */

/** 
 * @brief System and environment settings 
 *
 */
extern bool enableWeb;          /**< Enable or disable web file server */
extern int8_t tempOffset;       /**< BME temperature sensor offset */
extern bool calculateDST;       /**< Daylight Saving Time calculation flag */

/**
 * @brief Structure for map settings
 *
 * @details Contains configuration flags for displaying and interacting with the map UI,
 *          such as compass, speed, map type, and scale settings.
 */
struct MAP
{
    bool showMapCompass;    /**< Show compass in map screen */
    bool compassRotation;   /**< Enable compass rotation in map screen */
    bool mapRotationComp;   /**< Rotate map with compass heading */
    bool showMapSpeed;      /**< Show speed in map screen */
    bool vectorMap;         /**< Map type: true for vector, false for rendered */
    bool showMapScale;      /**< Show map scale on screen */
    bool fillPolygons;      /**< Flag for polygon filling */
};
extern MAP mapSet; /**< Global instance for map settings */

/**
 * @brief Structure for navigation settings
 *
 * @details Contains navigation configuration 
 */
struct NAVIGATION 
{
    bool simNavigation;     /**< Indicates whether navigation simulation mode is enabled or disabled. */
};
extern NAVIGATION navSet; /**< Global instance for navigation settings */

void loadPreferences();

void saveGPSBaud(uint16_t gpsBaud);
void saveGPSUpdateRate(uint16_t gpsUpdateRate);
void saveWidgetPos(char *widget, uint16_t posX, uint16_t posY);
void printSettings();