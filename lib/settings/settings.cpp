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
    preferences.begin("ICENAV", false);
    offX = preferences.getFloat("C_offset_x", 0.0);
    offY = preferences.getFloat("C_offset_y", 0.0);
    isMapRotation = preferences.getBool("Map_rot", false);
    defaultZoom = preferences.getUInt("Def_zoom", defZoom);
    zoom = defaultZoom;
    showMapCompass = preferences.getBool("Map_compass", false);
    isCompassRot = preferences.getBool("Compass_rot", true);
    showMapSpeed = preferences.getBool("Map_speed", false);
    showMapScale = preferences.getBool("Map_scale", false);
    gpsBaud = preferences.getShort("GPS_speed", 2);
    gpsUpdate = preferences.getShort("GPS_rate", 3);
    compassPosX = preferences.getInt("Compass_X", 60);
    compassPosY = preferences.getInt("Compass_Y", 82);
    coordPosX = preferences.getInt("Coords_X", 66);
    coordPosY = preferences.getInt("Coords_Y", 29);
    altitudePosX = preferences.getInt("Altitude_X", 8);
    altitudePosY = preferences.getInt("Altitude_Y", 293);
    speedPosX = preferences.getInt("Speed_X", 1);
    speedPosY = preferences.getInt("Speed_Y", 337);
    isVectorMap = preferences.getBool("Map_vector", false);
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

    preferences.end();
}

/**
 * @brief Save Map Rotation Type
 *
 * @param zoomRotation
 */
void saveMapRotation(bool zoomRotation)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_rot", zoomRotation);
    preferences.end();
}

/**
 * @brief Save current compass calibration in preferences
 *
 * @param offsetX
 * @param offsetY
 */
void saveCompassCal(float offsetX, float offsetY)
{
    preferences.begin("ICENAV", false);
    preferences.putFloat("C_offset_x", offsetX);
    preferences.putFloat("C_offset_y", offsetY);
    preferences.end();
}

/**
 * @brief Save default zoom value
 *
 * @param defaultZoom
 */
void saveDefaultZoom(uint8_t defaultZoom)
{
    preferences.begin("ICENAV", false);
    preferences.putUInt("Def_zoom", defaultZoom);
    preferences.end();
}

/**
 * @brief Save show compass in map
 *
 * @param showCompass
 */
void saveShowCompass(bool showCompass)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_compass", showCompass);
    preferences.end();
}

/**
 * @brief Save compass rotation in map
 *
 * @param compassRot
 */
void saveCompassRot(bool compassRot)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Compass_rot", compassRot);
    preferences.end();
}

/**
 * @brief Save show speed in map
 *
 * @param showSpeed
 */
void saveShowSpeed(bool showSpeed)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_speed", showSpeed);
    preferences.end();
}

/**
 * @brief Save show scale in map
 *
 * @param showScale
 */
void saveShowScale(bool showScale)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_scale", showScale);
    preferences.end();
}

/**
 * @brief Save GPS speed
 *
 * @param gpsBaud
 */
void saveGPSBaud(uint16_t gpsBaud)
{
    preferences.begin("ICENAV", false);
    preferences.putShort("GPS_speed", gpsBaud);
    preferences.end();
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
    preferences.begin("ICENAV", false);
    preferences.putShort("GPS_rate", gpsUpdateRate);
    preferences.end();
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

    preferences.begin("ICENAV", false);
    preferences.putInt(widgetX, posX);
    preferences.putInt(widgetY, posY);
    preferences.end();
}

/**
 * @brief Save Map Type
 *
 * @param vector
 */
void saveMapType(bool vector)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_vector", vector);
    preferences.end();
}