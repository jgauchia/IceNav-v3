/**
 * @file settings.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Settings functions
 * @version 0.2.4
 * @date 2025-12
 */

#include "settings.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "Settings";

MAP mapSet;
NAVIGATION navSet;

/**
 * @brief Zoom Levels and Default zoom
 *
 */
uint8_t minZoom = 0;         /**< Minimum Zoom Level */
uint8_t maxZoom = 0;         /**< Maximum Zoom Level */
uint8_t defZoom = 15;        /**< Default Zoom Level for rendered map */
uint8_t zoom = 0;            /**< Current Zoom Level */

#ifdef LARGE_SCREEN
    static const float scale = 1.0f;   /**< Scale factor for large screens */
#else
    static const float scale = 0.75f;  /**< Scale factor for small screens */
#endif

/**
 * @brief Global variables definition for device preferences & config.
 *
 */
uint8_t  defaultZoom    = 0;    /**< Default Zoom Value */
uint8_t  defBright      = 255;  /**< Default Brightness */
uint16_t gpsBaud        = 0;    /**< GPS Baud Rate */
uint16_t gpsUpdate      = 0;    /**< GPS Update Rate */
uint16_t compassPosX    = 0;    /**< Compass widget position X */
uint16_t compassPosY    = 0;    /**< Compass widget position Y */
uint16_t coordPosX      = 0;    /**< Coordinates widget position X */
uint16_t coordPosY      = 0;    /**< Coordinates widget position Y */
uint16_t altitudePosX   = 0;    /**< Altitude widget position X */
uint16_t altitudePosY   = 0;    /**< Altitude widget position Y */
uint16_t speedPosX      = 0;    /**< Speed widget position X */
uint16_t speedPosY      = 0;    /**< Speed widget position Y */
uint16_t sunPosX        = 0;    /**< Sunrise/sunset widget position X */
uint16_t sunPosY        = 0;    /**< Sunrise/sunset widget position Y */
bool     enableWeb      = true; /**< Enable or disable web file server */
bool     showMapToolBar = false;/**< Show Map Toolbar */
int8_t   tempOffset     = 0;    /**< BME Temperature offset */
bool     calculateDST   = false;/**< Daylight Saving Time calculation flag */


extern Battery battery;
extern Compass compass;
extern Gps gps;

/**
 * @brief Load stored preferences
 *
 * @details Loads all device configuration parameters from persistent storage.
 * 			Configures peripherals, map settings, widget positions, and feature flags
 * 			based on user or default preferences.
 */
void loadPreferences()
{
    cfg.init("ICENAV");
    #ifdef ENABLE_COMPASS
        compass.setOffsets(cfg.getFloat(PKEYS::KCOMP_OFFSET_X, 0.0f), cfg.getFloat(PKEYS::KCOMP_OFFSET_Y, 0.0f));
        compass.setDeclinationAngle(cfg.getFloat(PKEYS::KDECL_ANG, 0.22f));
        compass.enableKalmanFilter(cfg.getBool(PKEYS::KKALM_FIL, false));
        compass.setKalmanFilterConst(cfg.getFloat(PKEYS::KKALM_Q, 0.01f),cfg.getFloat(PKEYS::KKALM_R, 0.1f));
    #endif  
    mapSet.mapRotationComp = cfg.getBool(PKEYS::KMAP_ROT_MODE, false);
    mapSet.showMapCompass = cfg.getBool(PKEYS::KMAP_COMPASS, true);
    mapSet.compassRotation = cfg.getBool(PKEYS::KMAP_COMP_ROT, true);
    mapSet.showMapSpeed = cfg.getBool(PKEYS::KMAP_SPEED, true);
    mapSet.vectorMap = cfg.getBool(PKEYS::KMAP_VECTOR, false);
    mapSet.showMapScale = cfg.getBool(PKEYS::KMAP_SCALE, true);
    mapSet.fillPolygons = cfg.getBool(PKEYS::KFILL_POL, false);
    navSet.simNavigation = cfg.getBool(PKEYS::KSIM_NAV, false);
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
    minZoom = 6;
    maxZoom = 17;
    defaultZoom = cfg.getUInt(PKEYS::KDEF_ZOOM, defZoom);
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

    battery.setBatteryLevels(cfg.getFloat(PKEYS::KVMAX_BATT, 4.2f), cfg.getFloat(PKEYS::KVMIN_BATT, 3.6f));
    printSettings();
}

/**
 * @brief Save GPS baud rate setting
 *
 * @details Saves the GPS baud rate to persistent storage and configures the GPS port
 * 			accordingly. If not using auto baud detection, sends configuration commands
 * 			to the GPS module (AT6558D), resets the port, and sets the new baud rate.
 * 			If using auto baud detection (gpsBaud == 3), attempts to detect baud rate
 * 			automatically and reconfigures the port.
 *
 * @param gpsBaud Baud rate index to save and configure
 */
void saveGPSBaud(uint16_t gpsBaud)
{
    cfg.saveShort(PKEYS::KGPS_SPEED, gpsBaud);
    if (gpsBaud != 3)
    {
        #ifdef AT6558D_GPS
            gpsPort.flush();
            gpsPort.println(GPS_BAUD_PCAS[gpsBaud]);
            gpsPort.flush();
            gpsPort.println("$PCAS00*01\r\n");
            gpsPort.flush();
            vTaskDelay(pdMS_TO_TICKS(500));
        #endif
        gpsPort.flush();
        gpsPort.end();
        vTaskDelay(pdMS_TO_TICKS(500));
        gpsPort.setRxBufferSize(1024);
        gpsPort.begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    else
    {
        gpsBaudDetected = gps.autoBaud();

        if (gpsBaudDetected != 0)
        {
            gpsPort.flush();
            gpsPort.end();
            vTaskDelay(pdMS_TO_TICKS(500));
            gpsPort.setRxBufferSize(1024);
            gpsPort.begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
  }
}

/**
 * @brief Save GPS update rate
 *
 * @details Saves the GPS update rate to persistent storage and, if using the AT6558D GPS module,
 * 			sends the corresponding configuration commands to set the update rate.
 *
 * @param gpsUpdateRate Update rate index to save and configure
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
        vTaskDelay(pdMS_TO_TICKS(500));
    #endif
}

/**
 * @brief Save widget position
 *
 * @details Saves the X and Y coordinates of a given widget to persistent storage.
 *
 * @param widget Name of the widget (used as key prefix)
 * @param posX   X position to save
 * @param posY   Y position to save
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
 *
 * @details Prints all configuration keys, indicating if their values are custom or default, and displays their current values.
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
