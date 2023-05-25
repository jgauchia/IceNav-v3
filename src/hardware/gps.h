/**
 * @file gps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.1.4
 * @date 2023-05-23
 */

#include <TimeLib.h>
#include <TinyGPS++.h>

#define MAX_SATELLITES 60
#define MAX_SATELLLITES_IN_VIEW 32
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS;
bool is_gps_fixed = false;
uint8_t fix_mode_old = 0;
uint8_t fix_old = 0;
uint8_t sat_count_old = 0;

/**
 * @brief Custom NMEA sentences
 *
 */
#ifdef MULTI_GNSS

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GNGSA"), 15); // $GNGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GNGSA"), 16); // $GNGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GNGSA"), 17); // $GNGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GNGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GNGSA"), 2);

// GPS Satellites in view
TinyGPSCustom totalGPSMsg(GPS, PSTR("GPGSV"), 1); // $GPGSV sentence, first element
TinyGPSCustom msgGPSNum(GPS, PSTR("GPGSV"), 2);   // $GPGSV sentence, second element
TinyGPSCustom satGPSNum[4];                       // to be initialized later
TinyGPSCustom elevGPS[4];
TinyGPSCustom azimGPS[4];
TinyGPSCustom snrGPS[4];

// GLONASS Satellites in view
TinyGPSCustom totalGLMsg(GPS, PSTR("GLGSV"), 1); // $GLGSV sentence, first element
TinyGPSCustom msgGLNum(GPS, PSTR("GLGSV"), 2);   // $GLGSV sentence, second element
TinyGPSCustom satGLNum[4];                       // to be initialized later
TinyGPSCustom elevGL[4];
TinyGPSCustom azimGL[4];
TinyGPSCustom snrGL[4];

// BEIDOU Satellites in view
TinyGPSCustom totalBDMsg(GPS, PSTR("BDGSV"), 1); // $BDGSV sentence, first element
TinyGPSCustom msgBDNum(GPS, PSTR("BDGSV"), 2);   // $BDGSV sentence, second element
TinyGPSCustom satBDNum[4];                       // to be initialized later
TinyGPSCustom elevBD[4];
TinyGPSCustom azimBD[4];
TinyGPSCustom snrBD[4];

#else

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GPGSA"), 15); // $GPGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GPGSA"), 16); // $GPGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GPGSA"), 17); // $GPGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GPGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GPGSA"), 2);

// GPS Satellites in view
TinyGPSCustom totalGPSMsg(GPS, PSTR("GPGSV"), 1); // $GPGSV sentence, first element
TinyGPSCustom msgGPSNum(GPS, PSTR("GPGSV"), 2);   // $GPGSV sentence, second element
TinyGPSCustom satGPSNum[4];                       // to be initialized later
TinyGPSCustom elevGPS[4];
TinyGPSCustom azimGPS[4];
TinyGPSCustom snrGPS[4];

#endif

/**
 * @brief Structure for satellite position (number, elev, azimut,...)
 *
 */
struct
{
  bool active;
  uint8_t sat_num;
  uint8_t elev;
  uint16_t azim;
  uint8_t snr;
  uint16_t pos_x;
  uint16_t pos_y;
} sat_tracker[MAX_SATELLITES];

/**
 * @brief Init GPS and custon NMEA parsing
 *
 */
void init_gps()
{
  gps->begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX, GPS_TX);

#ifdef AT6558D_GPS
  // GPS
  // gps->println("$PCAS04,1*18\r\n");
  // GPS+GLONASS
  // gps->println("$PCAS04,5*1C\r\n");
  // GPS+BDS+GLONASS
  gps->println("$PCAS04,7*1E\r\n");
  gps->flush();
  delay(100);

  // 1Hz Update
  gps->println("$PCAS02,1000*2E\r\n");
  // 5Hz Update
  // gps->println("$PCAS02,200*1D\r\n");
  gps->flush();
  delay(100);

  // Set NMEA 4.1
  gps->println("$PCAS05,2*1A\r\n");
  gps->flush();
  delay(100);
#endif

  // Initialize satellites in view custom NMEA structure
  for (int i = 0; i < 4; ++i)
  {
    satGPSNum[i].begin(GPS, PSTR("GPGSV"), 4 + 4 * i); // offsets 4, 8, 12, 16
    elevGPS[i].begin(GPS, PSTR("GPGSV"), 5 + 4 * i);   // offsets 5, 9, 13, 17
    azimGPS[i].begin(GPS, PSTR("GPGSV"), 6 + 4 * i);   // offsets 6, 10, 14, 18
    snrGPS[i].begin(GPS, PSTR("GPGSV"), 7 + 4 * i);    // offsets 7, 11, 15, 19

#ifdef MULTI_GNSS

    satGLNum[i].begin(GPS, PSTR("GLGSV"), 4 + 4 * i); // offsets 4, 8, 12, 16
    elevGL[i].begin(GPS, PSTR("GLGSV"), 5 + 4 * i);   // offsets 5, 9, 13, 17
    azimGL[i].begin(GPS, PSTR("GLGSV"), 6 + 4 * i);   // offsets 6, 10, 14, 18
    snrGL[i].begin(GPS, PSTR("GLGSV"), 7 + 4 * i);    // offsets 7, 11, 15, 19

    satBDNum[i].begin(GPS, PSTR("BDGSV"), 4 + 4 * i); // offsets 4, 8, 12, 16
    elevBD[i].begin(GPS, PSTR("BDGSV"), 5 + 4 * i);   // offsets 5, 9, 13, 17
    azimBD[i].begin(GPS, PSTR("BDGSV"), 6 + 4 * i);   // offsets 6, 10, 14, 18
    snrBD[i].begin(GPS, PSTR("BDGSV"), 7 + 4 * i);    // offsets 7, 11, 15, 19

#endif
  }
}
