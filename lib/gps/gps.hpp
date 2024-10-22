/**
 * @file gps.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  GPS definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef GPS_HPP
#define GPS_HPP

#include <NMEAGPS.h>
#include <Streamers.h>
#include "settings.hpp"
#include "timezone.hpp"

extern uint8_t GPS_TX;
extern uint8_t GPS_RX;

#define MAX_SATELLITES 120
#define MAX_SATELLLITES_IN_VIEW 32

#define DEBUG_PORT Serial
#define gpsPort Serial2
#define GPS_PORT_NAME "Serial2"
extern gps_fix  fix;
extern NMEAGPS  GPS;
extern NeoGPS::time_t localTime;

extern bool isGpsFixed;
extern bool isTimeFixed;
extern long gpsBaudDetected;
extern bool nmea_output_enable;

static unsigned long GPS_BAUD[] = {4800, 9600, 19200, 38400, 0};
static const char *GPS_BAUD_PCAS[] = {"$PCAS01,0*1C\r\n", "$PCAS01,1*1D\r\n", "$PCAS01,2*1E\r\n", "$PCAS01,3*1F\r\n"};
static const char *GPS_RATE_PCAS[] = {"$PCAS02,1000*2E\r\n", "$PCAS02,500*1A\r\n", "$PCAS02,250*18\r\n", "$PCAS02,200*1D\r\n", "$PCAS02,100*1E\r\n"};

/**
 * @brief Structure for satellite position (number, elev, azimuth,...)
 *
 */
struct SV
{
  bool     active;
  uint8_t  satNum;
  uint8_t  elev;
  uint16_t azim;
  uint8_t  snr;
  uint16_t posX;
  uint16_t posY;
  char     talker_id[3];
};

extern SV satTracker[MAX_SATELLITES];

/**
 * @brief Structure for GPS Parsed Data
 *
 */
struct GPSDATA 
{
  uint8_t  satellites;
  uint8_t  fixMode;
  int16_t  altitude;
  uint16_t speed;
  double   latitude;
  double   longitude;
  uint16_t heading;
  float    hdop;
  float    pdop;
  float    vdop;
  uint8_t  satInView;
};

extern GPSDATA gpsData;

void initGPS();
double getLat();
double getLon();
void getGPSData();
long detectRate(int rxPin);
long autoBaudGPS();

#endif
