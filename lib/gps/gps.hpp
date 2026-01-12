/**
 * @file gps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "globalGpxDef.h"
#include <NMEAGPS.h>
#include <Streamers.h>
#include "settings.hpp"
#include <vector>


extern uint8_t GPS_TX; /**< GPS TX pin number. */
extern uint8_t GPS_RX; /**< GPS RX pin number. */

#define MAX_SATELLITES 120		   /**< Maximum number of satellites supported. */
#define MAX_SATELLLITES_IN_VIEW 32 /**< Maximum number of satellites in view. */

#define DEBUG_PORT Serial 		/**< Serial port used for debug output. */
#define gpsPort Serial2			/**< Serial port used for GPS communication. */
#define GPS_PORT_NAME "Serial2" /**< Name of the GPS serial port. */

extern gps_fix fix; /**< Latest parsed GPS fix data. */
extern NMEAGPS GPS; /**< NMEAGPS parser instance. */

extern bool setTime; /**< Indicates if time should be set from GPS. */
void calculateSun(); /**< Calculates sunrise and sunset times based on current GPS data. */

const char* getPosixTZ(const char* name); /**< Gets the POSIX time zone string for a given time zone name. @param name Time zone name (e.g., "CET", "UTC"). @return POSIX TZ string. */

extern bool isGpsFixed;		 /**< Indicates whether a valid GPS fix has been acquired. */
extern long gpsBaudDetected; 	 /**< Detected GPS baud rate. */
extern bool nmea_output_enable;  /**< Enables or disables NMEA output. */

class Gps;
extern Gps gps; /**< Global GPS instance */

static unsigned long GPS_BAUD[] = {4800, 9600, 19200, 0}; /**< Supported GPS baud rates. */
static const char *GPS_BAUD_PCAS[] = {"$PCAS01,0*1C\r\n", "$PCAS01,1*1D\r\n", "$PCAS01,2*1E\r\n"}; /**< NMEA command strings to set baud rate for PCAS modules. */
static const char *GPS_RATE_PCAS[] = {"$PCAS02,1000*2E\r\n", "$PCAS02,500*1A\r\n", "$PCAS02,250*18\r\n", "$PCAS02,200*1D\r\n", "$PCAS02,100*1E\r\n"}; /**< NMEA command strings to set update rate for PCAS modules. */

/**
 * @brief Satellite Constellation Canvas Definition
 *
 */
static const uint8_t canvasOffset = 15;          					    /**< Offset from the edge to start drawing the satellite constellation canvas */
static const uint8_t canvasSize = 180;							    /**< Total size (width and height) of the constellation canvas */
static const uint8_t canvasCenter_X = canvasSize / 2;				/**< X coordinate of the canvas center */
static const uint8_t canvasCenter_Y = canvasSize / 2;				/**< Y coordinate of the canvas center */
static const uint8_t canvasRadius = canvasCenter_X - canvasOffset;	/**< Radius of the drawable area for the constellation */


/**
 * @class Gps
 * @brief GPS management class using NeoGPS library.
 */
class Gps
{
    public:
        Gps();
        void init();
        void update();
        void setLocalTime(NeoGPS::time_t gpsTime, const char* tz);
        void simFakeGPS(const TrackVector& trackData, uint16_t speed, uint16_t refresh);
        float getLat();
        float getLon();
        void getGPSData();
        long detectRate(int rxPin);
        long autoBaud();
        bool isSpeedChanged();
        bool isAltitudeChanged();
        bool hasLocationChange();
        bool isDOPChanged();

        /**
        * @struct GPSDATA
        * @brief Holds parsed GPS data for easy access.
        */
        struct GPSDATA
        {
            uint8_t satellites;   /**< Number of satellites used for fix. */
            uint8_t fixMode;      /**< GPS fix mode. */
            int16_t altitude;     /**< Altitude in meters. */
            uint16_t speed;       /**< Speed in km/h or knots. */
            float latitude;       /**< Latitude in decimal degrees. */
            float longitude;      /**< Longitude in decimal degrees. */
            uint16_t heading;     /**< Heading in degrees. */
            float hdop;           /**< Horizontal dilution of precision. */
            float pdop;           /**< Position dilution of precision. */
            float vdop;           /**< Vertical dilution of precision. */
            uint8_t satInView;    /**< Number of satellites in view. */
            char sunriseHour[6];  /**< Sunrise time as string (HH:MM). */
            char sunsetHour[6];   /**< Sunset time as string (HH:MM). */
            int UTC;              /**< UTC offset. */
        } gpsData;

        /**
        * @struct SV
        * @brief Holds information about a tracked satellite.
        */
        struct SV
        {
            bool active;          /**< True if the satellite is active. */
            uint8_t satNum;       /**< Satellite number. */
            uint8_t elev;         /**< Elevation in degrees. */
            uint16_t azim;        /**< Azimuth in degrees. */
            uint8_t snr;          /**< Signal-to-noise ratio. */
            uint16_t posX;        /**< X position for display/map. */
            uint16_t posY;        /**< Y position for display/map. */
            char talker_id[3];    /**< NMEA talker ID. */
        } satTracker[MAX_SATELLITES];

    private:
        uint16_t previousSpeed;      /**< Previous speed value for change detection. */
        int16_t previousAltitude;    /**< Previous altitude value for change detection. */
        float previousLatitude;     /**< Previous latitude for change detection. */
        float previousLongitude;    /**< Previous longitude for change detection. */
        float previousHdop;          /**< Previous HDOP for change detection. */
        float previousPdop;          /**< Previous PDOP for change detection. */
        float previousVdop;          /**< Previous VDOP for change detection. */

        /**
        * @brief Variables for "fake" GPS signal from loaded track (simulation)
        * 
        */
        const float posAlpha = 0.6f;           /**< Position smoothing factor, range 0 (no smoothing) to 1 (full smoothing) */
        const float headAlpha = 0.5f;          /**< Heading smoothing factor, controls how fast heading adapts */
        const float minStepDist = 3.0f;        /**< Minimum distance in meters between simulation steps to update */
        const int headingLookahead = 5;        /**< Number of track points ahead used to calculate the heading */
        float smoothedLat = 0.0f;              /**< Smoothed latitude after filtering */
        float smoothedLon = 0.0f;              /**< Smoothed longitude after filtering */
        float filteredHeading = 0.0f;          /**< Smoothed heading after filtering */
        float lastSimLat = 0.0f;       		   /**< Last simulated latitude used for step distance */
        float lastSimLon = 0.0f;     	       /**< Last simulated longitude used for step distance */
        float accumulatedDist = 0.0f;          /**< Accumulated distance for multi-second simulation */
        int simulationIndex = 0;                   /**< Current index in track simulation */
        unsigned long lastSimulationTime = 0;      /**< Timestamp of last simulation update in milliseconds */
};
