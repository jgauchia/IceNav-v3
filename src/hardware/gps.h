/**
 * @file gps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.1.6
 * @date 2023-06-14
 */

#include <TimeLib.h>
#include <TinyGPS++.h>

#define MAX_SATELLITES 120
#define MAX_SATELLLITES_IN_VIEW 32
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS;
bool is_gps_fixed = false;
uint8_t fix_mode_old = 0;
uint8_t fix_old = 0;

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
#ifdef MULTI_GNSS

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GNGSA"), 15); // $GNGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GNGSA"), 16); // $GNGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GNGSA"), 17); // $GNGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GNGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GNGSA"), 2);

// GPS Satellites in view
GSV GPS_GSV;

// GLONASS Satellites in view
GSV GL_GSV;

// BEIDOU Satellites in view
GSV BD_GSV;

#else

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GPGSA"), 15); // $GPGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GPGSA"), 16); // $GPGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GPGSA"), 17); // $GPGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GPGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GPGSA"), 2);

// GPS Satellites in view
GSV GPS_GSV;

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

  GPS_GSV.totalMsg.begin(GPS, PSTR("GPGSV"), 1);
  GPS_GSV.msgNum.begin(GPS, PSTR("GPGSV"), 2);
  GPS_GSV.satsInView.begin(GPS, PSTR("GPGSV") ,3);

#ifdef MULTI_GNSS

  GL_GSV.totalMsg.begin(GPS, PSTR("GLGSV"), 1);
  GL_GSV.msgNum.begin(GPS, PSTR("GLGSV"), 2);
  GL_GSV.satsInView.begin(GPS, PSTR("GLGSV") ,3);

  BD_GSV.totalMsg.begin(GPS, PSTR("BDGSV"), 1);
  BD_GSV.msgNum.begin(GPS, PSTR("BDGSV"), 2);
  BD_GSV.satsInView.begin(GPS, PSTR("BDGSV") ,3);

#endif

  for (int i = 0; i < 4; ++i)
  {
    GPS_GSV.satNum[i].begin(GPS, PSTR("GPGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    GPS_GSV.elev[i].begin(GPS, PSTR("GPGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    GPS_GSV.azim[i].begin(GPS, PSTR("GPGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    GPS_GSV.snr[i].begin(GPS, PSTR("GPGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

#ifdef MULTI_GNSS

    GL_GSV.satNum[i].begin(GPS, PSTR("GLGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    GL_GSV.elev[i].begin(GPS, PSTR("GLGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    GL_GSV.azim[i].begin(GPS, PSTR("GLGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    GL_GSV.snr[i].begin(GPS, PSTR("GLGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

    BD_GSV.satNum[i].begin(GPS, PSTR("BDGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    BD_GSV.elev[i].begin(GPS, PSTR("BDGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    BD_GSV.azim[i].begin(GPS, PSTR("BDGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    BD_GSV.snr[i].begin(GPS, PSTR("BDGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

#endif
  }
}
