/**
 * @file gps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef GPS_HPP
#define GPS_HPP

#include <NMEAGPS.h>
#include <Streamers.h>
#include "settings.hpp"

#define DEG2RAD(a) ((a) / (180 / M_PI)) // Convert degrees to radians
#define RAD2DEG(a) ((a) * (180 / M_PI)) // Convert radians to degrees

extern uint8_t GPS_TX;
extern uint8_t GPS_RX;

#define MAX_SATELLITES 120
#define MAX_SATELLLITES_IN_VIEW 32

#define DEBUG_PORT Serial
#define gpsPort Serial2
#define GPS_PORT_NAME "Serial2"
extern gps_fix fix;
extern NMEAGPS GPS;

extern bool setTime;
void calculateSun();
const char* getPosixTZ(const char* name);

extern bool isGpsFixed;
extern bool isTimeFixed;
extern long gpsBaudDetected;
extern bool nmea_output_enable;

static unsigned long GPS_BAUD[] PROGMEM = {4800, 9600, 19200, 0};
static const char *GPS_BAUD_PCAS[] PROGMEM = {"$PCAS01,0*1C\r\n", "$PCAS01,1*1D\r\n", "$PCAS01,2*1E\r\n"};
static const char *GPS_RATE_PCAS[] PROGMEM = {"$PCAS02,1000*2E\r\n", "$PCAS02,500*1A\r\n", "$PCAS02,250*18\r\n", "$PCAS02,200*1D\r\n", "$PCAS02,100*1E\r\n"};

/**
 * @brief Satellite Constellation Canvas Definition
 *
 */
static const uint8_t canvasOffset = 15;
static const uint8_t canvasSize = 180;
static const uint8_t canvasCenter_X = canvasSize / 2;
static const uint8_t canvasCenter_Y = canvasSize / 2;
static const uint8_t canvasRadius = canvasCenter_X - canvasOffset;

class Gps
{
public:
  Gps();
  void init();
  double getLat();
  double getLon();
  void getGPSData();
  long detectRate(int rxPin);
  long autoBaud();
  bool isSpeedChanged();
  bool isAltitudeChanged();
  bool hasLocationChange();
  bool isDOPChanged();
  void setLocalTime(NeoGPS::time_t gpsTime, const char* tz);

  struct GPSDATA
  {
    uint8_t satellites;
    uint8_t fixMode;
    int16_t altitude;
    uint16_t speed;
    double latitude;
    double longitude;
    uint16_t heading;
    float hdop;
    float pdop;
    float vdop;
    uint8_t satInView;
    char sunriseHour[6];
    char sunsetHour[6];
    int UTC;
  } gpsData;

  struct SV
  {
    bool active;
    uint8_t satNum;
    uint8_t elev;
    uint16_t azim;
    uint8_t snr;
    uint16_t posX;
    uint16_t posY;
    char talker_id[3];
  } satTracker[MAX_SATELLITES];

private:
  uint16_t previousSpeed;
  int16_t previousAltitude;
  double previousLatitude; 
  double previousLongitude;
  float previousHdop; 
  float previousPdop; 
  float previousVdop; 
};

#endif 