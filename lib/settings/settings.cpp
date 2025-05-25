/**
 * @file settings.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Settings functions
 * @version 0.2.1
 * @date 2025-05
 */

#include "settings.hpp"

static const char* TAG PROGMEM = "Settings";

/**
 * @brief Structure for map settings
 *
 */
MAP mapSet;

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
uint8_t defaultZoom = 0;   // Default Zoom Value
uint8_t defBright = 255;   // Default Brightness
uint16_t gpsBaud = 0;      // GPS Speed
uint16_t gpsUpdate = 0;    // GPS Update rate
uint16_t compassPosX = 0;  // Compass widget position X
uint16_t compassPosY = 0;  // Compass widget position Y
uint16_t coordPosX = 0;    // Coordinates widget position X
uint16_t coordPosY = 0;    // Coordinates widget position Y
uint16_t altitudePosX = 0; // Altitude widget position X
uint16_t altitudePosY = 0; // Altitude widget position Y
uint16_t speedPosX = 0;    // Speed widget position X
uint16_t speedPosY = 0;    // Speed widget position Y
uint16_t sunPosX = 0;      // Sunrise/sunset position X
uint16_t sunPosY = 0;      // Sunrise/sunset position Y
bool enableWeb = true;     // Enable/disable web file server
bool showMapToolBar = false;  // Show Map Toolbar
int8_t tempOffset = 0;     // BME Temperature offset
extern Battery battery;
extern Compass compass;
extern Gps gps;
bool calculateDST = false; // Calculate DST flag

/**
 * @brief Load stored preferences
 *
 */
void loadPreferences()
{
  cfg.init("ICENAV");
#ifdef ENABLE_COMPASS
  compass.setOffsets(cfg.getFloat(PKEYS::KCOMP_OFFSET_X, 0.0), cfg.getFloat(PKEYS::KCOMP_OFFSET_Y, 0.0));
  compass.setDeclinationAngle(cfg.getFloat(PKEYS::KDECL_ANG, 0.22));
  compass.enableKalmanFilter(cfg.getBool(PKEYS::KKALM_FIL, false));
  compass.setKalmanFilterConst(cfg.getFloat(PKEYS::KKALM_Q, 0.01),cfg.getFloat(PKEYS::KKALM_R, 0.1));
#endif  
  mapSet.mapRotationComp = cfg.getBool(PKEYS::KMAP_ROT_MODE, false);
  mapSet.showMapCompass = cfg.getBool(PKEYS::KMAP_COMPASS, true);
  mapSet.compassRotation = cfg.getBool(PKEYS::KMAP_COMP_ROT, true);
  mapSet.mapFullScreen = cfg.getBool(PKEYS::KMAP_MODE, true);
  mapSet.showMapSpeed = cfg.getBool(PKEYS::KMAP_SPEED, true);
  mapSet.vectorMap = cfg.getBool(PKEYS::KMAP_VECTOR, false);
  mapSet.showMapScale = cfg.getBool(PKEYS::KMAP_SCALE, true);
  gpsBaud = cfg.getShort(PKEYS::KGPS_SPEED, 4);
  gpsUpdate = cfg.getShort(PKEYS::KGPS_RATE, 3);
  compassPosX = cfg.getInt(PKEYS::KCOMP_X, (TFT_WIDTH / 2) - (100 * scale));
  compassPosY = cfg.getInt(PKEYS::KCOMP_Y, 80);
  coordPosX = cfg.getInt(PKEYS::KCOORD_X, (TFT_WIDTH / 2) - (90 * scale));
  coordPosY = cfg.getInt(PKEYS::KCOORD_Y, 30);
  altitudePosX = cfg.getInt(PKEYS::KALTITUDE_X, 8);
  altitudePosY = cfg.getInt(PKEYS::KALTITUDE_Y, TFT_HEIGHT - 170);
  speedPosX = cfg.getInt(PKEYS::KSPEED_X, 1);
  speedPosY = cfg.getInt(PKEYS::KSPEED_Y, TFT_HEIGHT - 130);
  sunPosX = cfg.getInt(PKEYS::KSUN_X, 170);
  sunPosY = cfg.getInt(PKEYS::KSUN_Y, TFT_HEIGHT - 170);
  defBright = cfg.getUInt(PKEYS::KDEF_BRIGT, 254);
  if (mapSet.vectorMap)
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
  zoom = defaultZoom;
  GPS_TX = cfg.getUInt(PKEYS::KGPS_TX, GPS_TX);
  GPS_RX = cfg.getUInt(PKEYS::KGPS_RX, GPS_RX);
  enableWeb = cfg.getBool(PKEYS::KWEB_FILE, enableWeb);
  tempOffset = cfg.getInt(PKEYS::KTEMP_OFFS, 0);

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
  sunPosX = cfg.isKey(CONFKEYS::KSUN_X) ? cfg.getInt(CONFKEYS::KSPEED_X, speedPosX) : 3;
  sunPosY = cfg.isKey(CONFKEYS::KSUN_Y) ? cfg.getInt(CONFKEYS::KSPEED_Y, speedPosY) : 110;
#endif

  battery.setBatteryLevels(cfg.getFloat(PKEYS::KVMAX_BATT, 4.2), cfg.getFloat(PKEYS::KVMIN_BATT, 3.6));
  printSettings();
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
    gpsPort.flush();
    gpsPort.println(GPS_BAUD_PCAS[gpsBaud]);
    gpsPort.flush();
    gpsPort.println("$PCAS00*01\r\n");
    gpsPort.flush();
    delay(500);
#endif
    gpsPort.flush();
    gpsPort.end();
    delay(500);
    gpsPort.setRxBufferSize(1024);
    gpsPort.begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
    delay(500);
  }
  else
  {
    gpsBaudDetected = gps.autoBaud();

    if (gpsBaudDetected != 0)
    {
      gpsPort.flush();
      gpsPort.end();
      delay(500);
      gpsPort.setRxBufferSize(1024);
      gpsPort.begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
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
  gpsPort.flush();
  gpsPort.println(GPS_RATE_PCAS[gpsUpdateRate]);
  gpsPort.flush();
  gpsPort.println("$PCAS00*01\r\n");
  gpsPort.flush();
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
 * @brief Utility to show all settings
 */
void printSettings()
{
  ESP_LOGV(TAG, "%11s \t%s \t%s", "KEYNAME", "DEFINED", "VALUE");
  ESP_LOGV(TAG, "%11s \t%s \t%s", "=======", "=======", "=====");

  for (int i = 0; i < KCOUNT; i++)
  {
    if (i == PKEYS::KUSER)
      continue;
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined)
      value = cfg.getValue(key);
    ESP_LOGV(TAG, "%11s \t%s \t%s", key.c_str(), defined.c_str(), value.c_str());
  }
}
