/**
 * @file gps.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  GPS definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef GPS_HPP
#define GPS_HPP

#include <TinyGPS++.h>
#include "settings.hpp"

extern uint8_t GPS_TX;
extern uint8_t GPS_RX;

#define MAX_SATELLITES 120
#define MAX_SATELLLITES_IN_VIEW 32
extern HardwareSerial *gps;
extern TinyGPSPlus GPS;
extern bool isGpsFixed;
extern bool isTimeFixed;
extern long gpsBaudDetected;
extern bool nmea_output_enable;

static uint8_t fix_old = 0;
static unsigned long GPS_BAUD[] = {4800, 9600, 19200, 38400, 0};
static const char *GPS_BAUD_PCAS[] = {"$PCAS01,0*1C\r\n", "$PCAS01,1*1D\r\n", "$PCAS01,2*1E\r\n", "$PCAS01,3*1F\r\n"};
static const char *GPS_RATE_PCAS[] = {"$PCAS02,1000*2E\r\n", "$PCAS02,500*1A\r\n", "$PCAS02,250*18\r\n", "$PCAS02,200*1D\r\n", "$PCAS02,100*1E\r\n"};

/**
 * @brief Common Structure for satellites in view NMEA sentence
 *
 */
struct GSV
{
  TinyGPSCustom totalMsg;
  TinyGPSCustom msgNum;
  TinyGPSCustom satsInView;
  TinyGPSCustom satNum[4];
  TinyGPSCustom elev[4];
  TinyGPSCustom azim[4];
  TinyGPSCustom snr[4];
};

/**
 * @brief Custom NMEA sentences
 *
 */
#ifdef AT6558D_GPS

// DOP and fix mode
  extern TinyGPSCustom pdop; // $GNGSA sentence, 15th element
  extern TinyGPSCustom hdop; // $GNGSA sentence, 16th element
  extern TinyGPSCustom vdop; // $GNGSA sentence, 17th element
  extern TinyGPSCustom fixMode;

  extern GSV GPS_GSV; // GPS Satellites in view
  extern GSV GL_GSV;  // GLONASS Satellites in view
  extern GSV BD_GSV;  // BEIDOU Satellites in view

#else

  // DOP and fix mode
  extern TinyGPSCustom pdop; // $GPGSA sentence, 15th element
  extern TinyGPSCustom hdop; // $GPGSA sentence, 16th element
  extern TinyGPSCustom vdop; // $GPGSA sentence, 17th element
  extern TinyGPSCustom fixMode;

  extern GSV GPS_GSV; // GPS Satellites in view

#endif

/**
 * @brief Structure for satellite position (number, elev, azimuth,...)
 *
 */
struct
{
  bool active;
  uint8_t satNum;
  uint8_t elev;
  uint16_t azim;
  uint8_t snr;
  uint16_t posX;
  uint16_t posY;
} satTracker[MAX_SATELLITES];

void initGPS();
double getLat();
double getLon();
long detectRate(int rxPin);
long autoBaudGPS();

#endif
