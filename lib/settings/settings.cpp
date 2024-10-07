/**
 * @file settings.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Settings functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "settings.hpp"

/**
 * @brief Zoom Levels and Default zoom
 *
 */
uint8_t minZoom = 0;        // Min Zoom Level
uint8_t maxZoom = 0;        // Max Zoom Level
uint8_t defZoomRender = 15; // Default Zoom Level for render map
uint8_t defZoomVector = 2;  // Default Zoom Level for vector map
uint8_t zoom = 0;           // Actual Zoom Level

#ifdef LARGE_SCREEN
  static const float scale = 1.0f;
#else
  static const float scale = 0.75f;
#endif

/**
 * @brief Global Variables definition for device preferences & config.
 *
 */
bool isMapRotation = true;    // Map Compass Rotation
uint8_t defaultZoom = 0;      // Default Zoom Value
bool showMapCompass = true;   // Compass in map screen
bool isCompassRot = true;     // Compass rotation in map screen
bool showMapSpeed = true;     // Speed in map screen
bool showMapScale = true;     // Scale in map screen
bool isVectorMap = false;     // Map Type
bool isMapFullScreen = false; // Is Map Full Screen
uint16_t gpsBaud = 0;         // GPS Speed
uint16_t gpsUpdate = 0;       // GPS Update rate
uint16_t compassPosX = 0;     // Compass widget position X
uint16_t compassPosY = 0;     // Compass widget position Y
uint16_t coordPosX = 0;       // Coordinates widget position X
uint16_t coordPosY = 0;       // Coordinates widget position Y
uint16_t altitudePosX = 0;    // Altitude widget position X
uint16_t altitudePosY = 0;    // Altitude widget position Y
uint16_t speedPosX = 0;       // Speed widget position X
uint16_t speedPosY = 0;       // Speed widget position Y
bool enableWeb = true;        // Enable/disable web file server
bool showToolBar = false;     // Show Map Toolbar

/**
 * @brief Load stored preferences
 *
 */
void loadPreferences()
{
  cfg.init("ICENAV");
#ifdef ENABLE_COMPASS
  offX = cfg.getFloat(PKEYS::KCOMP_OFFSET_X, 0.0);
  offY = cfg.getFloat(PKEYS::KCOMP_OFFSET_Y, 0.0);
#endif
  isMapRotation = cfg.getBool(PKEYS::KMAP_ROT, false);
  zoom = defaultZoom;
  showMapCompass = cfg.getBool(PKEYS::KMAP_COMPASS, true);
  isCompassRot = cfg.getBool(PKEYS::KCOMP_ROT, true);
  showMapSpeed = cfg.getBool(PKEYS::KMAP_SPEED, true);
  showMapScale = cfg.getBool(PKEYS::KMAP_SCALE, true);
  gpsBaud = cfg.getShort(PKEYS::KGPS_SPEED, 4);
  gpsUpdate = cfg.getShort(PKEYS::KGPS_RATE, 3);
  compassPosX = cfg.getInt(PKEYS::KCOMP_X, ( TFT_WIDTH / 2 ) - ( 100 * scale ) );
  compassPosY = cfg.getInt(PKEYS::KCOMP_Y, 80);
  coordPosX = cfg.getInt(PKEYS::KCOORD_X, ( TFT_WIDTH / 2 ) - ( 90 * scale ) );
  coordPosY = cfg.getInt(PKEYS::KCOORD_Y, 30);
  altitudePosX = cfg.getInt(PKEYS::KALTITUDE_X, 8);
  altitudePosY = cfg.getInt(PKEYS::KALTITUDE_Y, TFT_HEIGHT - 170);
  speedPosX = cfg.getInt(PKEYS::KSPEED_X, 1);
  speedPosY = cfg.getInt(PKEYS::KSPEED_Y, TFT_HEIGHT - 130);
  isVectorMap = cfg.getBool(PKEYS::KMAP_VECTOR, false);
  if (isVectorMap)
  {
    minZoom = 1;
    maxZoom = 4;
    defaultZoom = cfg.getUInt(PKEYS::KDEF_ZOOM, defZoomVector);
  }
  else
  {
    minZoom = 6;
    maxZoom = 17;
    defaultZoom = cfg.getUInt(PKEYS::KDEF_ZOOM, defZoomRender);
  }
  isMapFullScreen = cfg.getBool(PKEYS::KMAP_MODE, true);
  GPS_TX = cfg.getUInt(PKEYS::KGPS_TX, GPS_TX);
  GPS_RX = cfg.getUInt(PKEYS::KGPS_RX, GPS_RX);
  enableWeb = cfg.getBool(PKEYS::KWEB_FILE, enableWeb);

  // Default Widgets positions
  #ifdef TDECK_ESP32S3
  compassPosX = cfg.isKey(CONFKEYS::KCOMP_X) ? cfg.getInt(CONFKEYS::KCOMP_X, compassPosX) : 162;
  compassPosY = cfg.isKey(CONFKEYS::KCOMP_Y) ? cfg.getInt(CONFKEYS::KCOMP_Y, compassPosY) : 6;
  coordPosX = cfg.isKey(CONFKEYS::KCOORD_X) ? cfg.getInt(CONFKEYS::KCOORD_X, coordPosX) : 1;
  coordPosY = cfg.isKey(CONFKEYS::KCOORD_Y) ? cfg.getInt(CONFKEYS::KCOORD_Y, coordPosY) : 10;
  altitudePosX = cfg.isKey(CONFKEYS::KALTITUDE_X) ? cfg.getInt(CONFKEYS::KALTITUDE_X, altitudePosX) : 5;
  altitudePosY = cfg.isKey(CONFKEYS::KALTITUDE_Y) ? cfg.getInt(CONFKEYS::KALTITUDE_Y, altitudePosY) : 57;
  speedPosX = cfg.isKey(CONFKEYS::KSPEED_X) ? cfg.getInt(CONFKEYS::KSPEED_X, speedPosX) : 3;
  speedPosY = cfg.isKey(CONFKEYS::KSPEED_Y) ? cfg.getInt(CONFKEYS::KSPEED_Y, speedPosY) : 94;
  #endif

  // compassPosX = 60;
  // compassPosY = 82;
  // coordPosX = 66;
  // coordPosY = 29;
  // altitudePosX = 8;
  // altitudePosY = 293;
  // speedPosX = 1;
  // speedPosY = 337;

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
  if (gpsBaud != 4)
  {
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
  else
  {
    gpsBaudDetected = autoBaudGPS();

    if (gpsBaudDetected != 0)
    {
      gps->flush();
      gps->end();
      delay(500);
      gps->begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
      delay(500);
    }
  }
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
 * @brief Save Map Mode
 *
 * @param mapMOde
 */
void saveShowMap(bool mapMode)
{
  cfg.saveBool(PKEYS::KMAP_MODE, mapMode);
}

/**
 * @brief Save GPS GPIO's
 *
 * @param txGpio
 * @param rxGpio
 */
void saveGpsGpio(int8_t txGpio, int8_t rxGpio)
{
  if (txGpio != -1)
    cfg.saveUInt(PKEYS::KGPS_TX, (uint8_t)txGpio);
  if (rxGpio != -1)
    cfg.saveUInt(PKEYS::KGPS_RX, (uint8_t)rxGpio);
}

/**
 * @brief Save Enable/disable web file server
 *
 * @param status 
 */
void saveWebFile(bool status)
{
  cfg.saveBool(PKEYS::KWEB_FILE, status);
}

/**
 * @brief Utility to show all settings
 */
void printSettings() 
{
  log_v("%11s \t%s \t%s", "KEYNAME", "DEFINED", "VALUE");
  log_v("%11s \t%s \t%s", "=======", "=======", "=====");

  for (int i = 0; i < KCOUNT; i++) {
    if (i == PKEYS::KUSER) continue;
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = cfg.getValue(key);
    log_v("%11s \t%s \t%s", key.c_str(), defined.c_str(), value.c_str());
  }
}
