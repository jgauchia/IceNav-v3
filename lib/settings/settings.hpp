/**
 * @file settings.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Settings functions
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include <EasyPreferences.hpp>
#include <NMEAGPS.h>
#include "tft.hpp"
#include "gps.hpp"
#include "battery.hpp"
#include "compass.hpp"

extern uint8_t minZoom;        // Min Zoom Level
extern uint8_t maxZoom;        // Max Zoom Level
extern uint8_t defZoomRender;  // Default Zoom Level for render map
extern uint8_t defZoomVector;  // Default Zoom Level for vector map
extern uint8_t zoom;           // Actual Zoom Level
extern uint8_t defBright;      // Default brightness
extern uint8_t defaultZoom;   // Default Zoom Value

extern bool showMapToolBar;      // Show Map Toolbar
extern uint16_t gpsBaud;      // GPS Speed
extern uint16_t gpsUpdate;    // GPS Update rate
extern uint16_t compassPosX;  // Compass widget position X
extern uint16_t compassPosY;  // Compass widget position Y
extern uint16_t coordPosX;    // Coordinates widget position X
extern uint16_t coordPosY;    // Coordinates widget position Y
extern uint16_t altitudePosX; // Altitude widget position X
extern uint16_t altitudePosY; // Altitude widget position Y
extern uint16_t speedPosX;    // Speed widget position X
extern uint16_t speedPosY;    // Speed widget position Y
extern uint16_t sunPosX;      // Sunrise/sunset position X
extern uint16_t sunPosY;      // Sunrise/sunset position Y
extern bool enableWeb;        // Enable/disable web file server
extern int8_t tempOffset;     // BME Temperature offset
extern bool calculateDST;     // Calculate DST flag


/**
 * @brief Structure for map settings
 *
 */
struct MAP
{
  bool showMapCompass;   // Compass in map screen
  bool compassRotation;  // Compass rotation in map screen
  bool mapRotationComp;  // Rotate map with compass
  bool mapFullScreen;    // Full Screen map
  bool showMapSpeed;     // Speed in map screen
  bool vectorMap;        // Map type (vector/render)
  bool showMapScale;     // Scale in map screen
};
extern MAP mapSet;


void loadPreferences();

void saveGPSBaud(uint16_t gpsBaud);
void saveGPSUpdateRate(uint16_t gpsUpdateRate);
void saveWidgetPos(char *widget, uint16_t posX, uint16_t posY);
void printSettings();