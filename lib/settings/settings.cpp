/**
 * @file settings.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Settings functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "settings.hpp"

/**
 * @brief Zoom Levels and Default zoom
 *
 */
uint8_t minZoom = 0; // Min Zoom Level
uint8_t maxZoom = 0; // Max Zoom Level
uint8_t defZoom = 2; // Default Zoom Level
uint8_t zoom = 0;    // Actual Zoom Level

/**
 * @brief Global Variables definition for device preferences & config.
 *
 */
bool isMapRotation = true;  // Map Compass Rotation
uint8_t defaultZoom = 0;    // Default Zoom Value
bool showMapCompass = true; // Compass in map screen
bool isCompassRot = true;   // Compass rotation in map screen
bool showMapSpeed = true;   // Speed in map screen
bool showMapScale = true;   // Scale in map screen
bool isVectorMap = false;   // Map Type
uint16_t gpsBaud = 0;       // GPS Speed
uint16_t gpsUpdate = 0;     // GPS Update rate
uint16_t compassPosX = 0;   // Compass widget position X
uint16_t compassPosY = 0;   // Compass widget position Y
uint16_t coordPosX = 0;     // Coordinates widget position X
uint16_t coordPosY = 0;     // Coordinates widget position Y
uint16_t altitudePosX = 0;  // Altitude widget position X
uint16_t altitudePosY = 0;  // Altitude widget position Y
uint16_t speedPosX = 0;     // Speed widget position X
uint16_t speedPosY = 0;     // Speed widget position Y

/**
 * @brief Load stored preferences
 *
 */
void loadPreferences()
{
    cfg.init("ICENAV");
    offX = cfg.getFloat(PKEYS::KCOMP_OFFSET_X, 0.0);
    offY = cfg.getFloat(PKEYS::KCOMP_OFFSET_Y, 0.0);
    isMapRotation = cfg.getBool(PKEYS::KMAP_ROT, false);
    defaultZoom = cfg.getUInt(PKEYS::KDEF_ZOOM, defZoom);
    zoom = defaultZoom;
    showMapCompass = cfg.getBool(PKEYS::KMAP_COMPASS, false);
    isCompassRot = cfg.getBool(PKEYS::KCOMP_ROT, true);
    showMapSpeed = cfg.getBool(PKEYS::KMAP_SPEED, false);
    showMapScale = cfg.getBool(PKEYS::KMAP_SCALE, false);
    gpsBaud = cfg.getShort(PKEYS::KGPS_SPEED, 2);
    gpsUpdate = cfg.getShort(PKEYS::KGPS_RATE, 3);
    compassPosX = cfg.getInt(PKEYS::KCOMP_X, 60);
    compassPosY = cfg.getInt(PKEYS::KCOMP_Y, 82);
    coordPosX = cfg.getInt(PKEYS::KCOORD_X, 66);
    coordPosY = cfg.getInt(PKEYS::KCOORD_Y, 29);
    altitudePosX = cfg.getInt(PKEYS::KALTITUDE_X, 8);
    altitudePosY = cfg.getInt(PKEYS::KALTITUDE_Y, 293);
    speedPosX = cfg.getInt(PKEYS::KSPEED_X, 1);
    speedPosY = cfg.getInt(PKEYS::KSPEED_Y, 337);
    isVectorMap = cfg.getBool(PKEYS::KMAP_VECTOR, false);
    if (isVectorMap)
    {
        minZoom = 1;
        maxZoom = 4;
    }
    else
    {
        minZoom = 6;
        maxZoom = 17;
    }

    // // Default Widgets positions
    // compassPosX = 60;
    // compassPosY = 82;
    // coordPosX = 66;
    // coordPosY = 29;
    // altitudePosX = 8;
    // altitudePosY = 293;
    // speedPosX = 1;
    // speedPosY = 337;

    log_v("COMPASS OFFSET X  %f", offX);
    log_v("COMPASS OFFSET Y  %f", offY);
    log_v("MAP ROTATION %d", isMapRotation);
    log_v("DEFAULT ZOOM LEVEL %d", zoom);
    log_v("SHOW MAP COMPASS %d", showMapCompass);
    log_v("MAP COMPASS ROT. %d", isCompassRot);
    log_v("SHOW MAP SPEED %d", showMapSpeed);
    log_v("SHOW MAP SCALE %d", showMapScale);
    log_v("GPS SPEED %d", gpsBaud);
    log_v("GPS UPDATE RATE %d", gpsUpdate);
    log_v("COMPASS POS X %d", compassPosX);
    log_v("COMPASS POS Y %d", compassPosY);
    log_v("COORDINATE POS X %d", coordPosX);
    log_v("COORDINATE POS Y %d", coordPosY);
    log_v("SPEED POS X %d", speedPosX);
    log_v("SPEED POS Y %d", speedPosY);
    log_v("ALTITUDE POS X %d", altitudePosX);
    log_v("ALTITUDE POS Y %d", altitudePosY);
    log_v("VECTOR MAP %d", isVectorMap);

    printSettings();
}

/**
 * @brief Save Map Rotation Type
 *
 * @param zoomRotation
 */
void saveMapRotation(bool zoomRotation)
{
    cfg.saveBool(PKEYS::KMAP_ROT, zoomRotation);
}

/**
 * @brief Save current compass calibration in preferences
 *
 * @param offsetX
 * @param offsetY
 */
void saveCompassCal(float offsetX, float offsetY)
{
    cfg.saveFloat(PKEYS::KCOMP_OFFSET_X, offsetX);
    cfg.saveFloat(PKEYS::KCOMP_OFFSET_Y, offsetY);
}

/**
 * @brief Save default zoom value
 *
 * @param defaultZoom
 */
void saveDefaultZoom(uint8_t defaultZoom)
{
    cfg.saveUInt(PKEYS::KDEF_ZOOM, defaultZoom);
}

/**
 * @brief Save show compass in map
 *
 * @param showCompass
 */
void saveShowCompass(bool showCompass)
{
    cfg.saveBool(PKEYS::KMAP_COMPASS, showCompass);
}

/**
 * @brief Save compass rotation in map
 *
 * @param compassRot
 */
void saveCompassRot(bool compassRot)
{
    cfg.saveBool(PKEYS::KCOMP_ROT, compassRot);
}

/**
 * @brief Save show speed in map
 *
 * @param showSpeed
 */
void saveShowSpeed(bool showSpeed)
{
    cfg.saveBool(PKEYS::KMAP_SPEED, showSpeed);
}

/**
 * @brief Save show scale in map
 *
 * @param showScale
 */
void saveShowScale(bool showScale)
{
    cfg.saveBool(PKEYS::KMAP_SCALE, showScale);
}

/**
 * @brief Save GPS speed
 *
 * @param gpsBaud
 */
void saveGPSBaud(uint16_t gpsBaud)
{
    cfg.saveShort(PKEYS::KGPS_SPEED, gpsBaud);
#ifdef AT6558D_GPS
    gps->flush();
    gps->println(GPS_BAUD_PCAS[gpsBaud]);
    gps->flush();
    gps->println("$PCAS00*01\r\n");
    gps->flush();
    delay(500);
#endif
    gps->flush();
    gps->end();
    delay(500);
    gps->begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
    delay(500);
}

/**
 * @brief Save GPS Update rate
 *
 * @param gpsUpdateRate
 */
void saveGPSUpdateRate(uint16_t gpsUpdateRate)
{
    cfg.saveShort(PKEYS::KGPS_RATE, gpsUpdateRate);
#ifdef AT6558D_GPS
    gps->flush();
    gps->println(GPS_RATE_PCAS[gpsUpdateRate]);
    gps->flush();
    gps->println("$PCAS00*01\r\n");
    gps->flush();
    delay(500);
#endif
}

/**
 * @brief Save Widget position
 *
 * @param posX
 * @param posY
 */
void saveWidgetPos(char *widget, uint16_t posX, uint16_t posY)
{
    const char *strX = "X";
    const char *strY = "Y";
    char widgetX[30] = "";
    char widgetY[30] = "";
    strcat(widgetX, widget);
    strcat(widgetX, strX);
    strcat(widgetY, widget);
    strcat(widgetY, strY);

    cfg.saveInt(widgetX, posX);
    cfg.saveInt(widgetY, posY);
}

/**
 * @brief Save Map Type
 *
 * @param vector
 */
void saveMapType(bool vector)
{
    cfg.saveBool(PKEYS::KMAP_VECTOR, vector);
}

/**
 * @brief Utility to show all settings
 */
void printSettings() 
{
  log_v("%11s \t%s \t%s", "KEYNAME", "DEFINED", "VALUE");
  log_v("%11s \t%s \t%s", "=======", "=======", "=====");

  for (int i = 0; i < KCOUNT; i++) {
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = cfg.getValue(key);
    log_v("%11s \t%s \t%s", key.c_str(), defined.c_str(), value.c_str());
  }
}