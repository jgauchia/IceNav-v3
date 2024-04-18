/**
 * @file preferences.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Preferences functions
 * @version 0.1.8
 * @date 2024-04
 */

#include <Preferences.h>

Preferences preferences;

/**
 * @brief Zoom Levels and Default zoom
 *
 */
uint8_t MIN_ZOOM = 0;
uint8_t MAX_ZOOM = 0;
uint8_t DEF_ZOOM = 2;
uint8_t zoom = 0;

/**
 * @brief Global Variables definition for device preferences & config.
 *
 */
float offX = 0.0, offY = 0.0; // Compass offset calibration
bool isMapRotation = true;     // Map Compass Rotation
uint8_t defaultZoom = 0;         // Default Zoom Value
bool showMapCompass = true; // Compass in map screen
bool showMapSpeed = true;   // Speed in map screen
bool showMapScale = true;   // Scale in map screen
// uint16_t gpsBaud = 0;       // GPS Speed (see gps.h)
// uint16_t gpsUpdate = 0;      // GPS Update rate (see gps.h)
int compassPosX = 0;
int compassPosY = 0;
int coordPosX = 0;
int coordPosY = 0;
int altitudePosX = 0;
int altitudePosY = 0;
int speedPosX = 0;
int speedPosY = 0;
bool isVectorMap = false;

/**
 * @brief Load stored preferences
 *
 */
static void loadPreferences()
{
    preferences.begin("ICENAV", false);
    offX = preferences.getFloat("C_offset_x", 0.0);
    offY = preferences.getFloat("C_offset_y", 0.0);
    isMapRotation = preferences.getBool("Map_rot", false);
    defaultZoom = preferences.getUInt("Def_zoom", DEF_ZOOM);
    zoom = defaultZoom;
    showMapCompass = preferences.getBool("Map_compass", false);
    showMapSpeed = preferences.getBool("Map_speed", false);
    showMapScale = preferences.getBool("Map_scale", false);
    gpsBaud = preferences.getShort("GPS_speed", 2);
    gpsUpdate = preferences.getShort("GPS_rate", 3);
    compassPosX = preferences.getInt("Compass_X", (TFT_WIDTH / 2) - 100);
    compassPosY = preferences.getInt("Compass_Y", (TFT_HEIGHT / 2) - 60);
    coordPosX = preferences.getInt("Coords_X", 15);
    coordPosY = preferences.getInt("Coords_Y", 10);
    altitudePosX = preferences.getInt("Altitude_X", 15);
    altitudePosY = preferences.getInt("Altitude_Y", 55);
    speedPosX = preferences.getInt("Speed_X", 15);
    speedPosY = preferences.getInt("Speed_Y", 100);
    isVectorMap = preferences.getBool("Map_vector", false);
    if (isVectorMap)
    {
        MIN_ZOOM = 1;
        MAX_ZOOM = 4;
    }
    else
    {
        MIN_ZOOM = 6;
        MAX_ZOOM = 17;
    }

    // // Default Widgets positions
    // compassPosX = (TFT_WIDTH / 2) - 100;
    // compassPosY = (TFT_HEIGHT / 2) - 60;
    // coordPosX = 15;
    // coordPosY = 10;
    // altitudePosX = 15;
    // altitudePosY = 55;
    // speedPosX = 15;
    // speedPosY = 100;

    log_v("COMPASS OFFSET X  %f", offX);
    log_v("COMPASS OFFSET Y  %f", offY);
    log_v("MAP ROTATION %d", isMapRotation);
    log_v("DEFAULT ZOOM LEVEL %d", zoom);
    log_v("SHOW MAP COMPASS %d", showMapCompass);
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
static void saveMapRotation(bool zoomRotation)
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
static void saveCompassCal(float offsetX, float offsetY)
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
static void saveDefaultZoom(uint8_t defaultZoom)
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
static void saveShowCompass(bool showCompass)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_compass", showCompass);
    preferences.end();
}

/**
 * @brief Save show speed in map
 *
 * @param showSpeed
 */
static void saveShowSpeed(bool showSpeed)
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
static void saveShowScale(bool showScale)
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
static void saveGPSBaud(uint16_t gpsBaud)
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
static void saveGPSUpdateRate(uint16_t gpsUpdateRate)
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
 * @brief Save Compass Widget position
 *
 * @param posX
 * @param posY
 */
static void saveWidgetPos(char *widget, int posX, int posY)
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
static void saveMapType(bool vector)
{
    preferences.begin("ICENAV", false);
    preferences.putBool("Map_vector", vector);
    preferences.end();
}
