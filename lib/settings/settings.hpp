/**
 * @file settings.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Settings functions
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <EasyPreferences.hpp>
#include <NMEAGPS.h>
#include "gps.hpp"
#include "battery.hpp"
#include "compass.hpp"

extern uint8_t minZoom;        // Min Zoom Level
extern uint8_t maxZoom;        // Max Zoom Level
extern uint8_t defZoomRender;  // Default Zoom Level for render map
extern uint8_t defZoomVector;  // Default Zoom Level for vector map
extern uint8_t zoom;           // Actual Zoom Level
extern uint8_t defBright;      // Default brightness

extern bool isMapRotation;    // Map Compass Rotation
extern uint8_t defaultZoom;   // Default Zoom Value
extern bool showMapCompass;   // Compass in map screen
extern bool isCompassRot;     // Compass rotation in map screen
extern bool showMapSpeed;     // Speed in map screen
extern bool showMapScale;     // Scale in map screen
extern bool isVectorMap;      // Map type
extern bool isMapFullScreen;  // Is Map Full Screen
extern bool showToolBar;      // Show Map Toolbar
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
extern String defDST;         // default DST zone
extern bool calculateDST;     // Calculate DST flag

void loadPreferences();
void saveMapRotation(bool zoomRotation);
void saveCompassCal(float offsetX, float offsetY);
void saveDefaultZoom(uint8_t defaultZoom);
void saveShowCompass(bool showCompass);
void saveCompassRot(bool compassRot);
void saveShowSpeed(bool showSpeed);
void saveShowScale(bool showScale);
void saveGPSBaud(uint16_t gpsBaud);
void saveGPSUpdateRate(uint16_t gpsUpdateRate);
void saveWidgetPos(char *widget, uint16_t posX, uint16_t posY);
void saveMapType(bool vector);
void saveShowMap(bool mapMode);
void saveGpsGpio(int8_t txGpio, int8_t rxGpio);
void saveWebFile(bool status);
void saveBrightness(uint8_t vb);
void printSettings();

#endif
