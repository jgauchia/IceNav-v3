/**
 * @file gps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.1.3
 * @date 2023-05-10
 */

#include <TimeLib.h>
#include <TinyGPS++.h>

#define MAX_SATELLITES 60
#define MAX_SATELLLITES_IN_VIEW 32
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS;
bool is_gps_fixed = false;
uint16_t upd_rate = 500;
uint8_t fix_mode_old = 0;
uint8_t fix_old = 0;
uint8_t sat_count_old = 0;

/**
 * @brief Custom NMEA sentences
 *
 */
TinyGPSCustom totalGPSMsg(GPS, PSTR("GPGSV"), 1); // $GPGSV sentence, first element
TinyGPSCustom msgGPSNum(GPS, PSTR("GPGSV"), 2);   // $GPGSV sentence, second element
TinyGPSCustom satGPSNum[4];                       // to be initialized later
TinyGPSCustom elevGPS[4];
TinyGPSCustom aziGPS[4];
TinyGPSCustom snrGPS[4];

#ifdef CUSTOMBOARD
TinyGPSCustom pdop(GPS, PSTR("GNGSA"), 15); // $GNGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GNGSA"), 16); // $GNGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GNGSA"), 17); // $GNGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GNGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GNGSA"), 2);
#else
TinyGPSCustom pdop(GPS, PSTR("GPGSA"), 15); // $GPGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GPGSA"), 16); // $GPGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GPGSA"), 17); // $GPGSA sentence, 17th element
TinyGPSCustom fix(GPS, PSTR("GPGGA"), 6);
TinyGPSCustom fix_mode(GPS, PSTR("GPGSA"), 2);
#endif

/**
 * @brief Structure for satellite position (elevGPS, azimut,...)
 *
 */
struct
{
  bool active;
  uint8_t satGPSNum;
  uint8_t elevGPS;
  uint8_t aziGPS;
  uint8_t snrGPS;
  uint16_t pos_x;
  uint16_t pos_y;
} sat_tracker[MAX_SATELLITES];

/**
 * @brief Init GPS and custon NMEA parsing
 *
 */
void init_gps()
{
  gps->begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX, GPS_TX);  // TODO: maybe should put into GPS ENABLE/DISABLE
#ifdef CUSTOMBOARD
  gps->println("$PCAS01,3*1F\r\n");
  delay(100);
  gps->flush();
  gps->end();
  gps->begin(38400, SERIAL_8N1, GPS_RX, GPS_TX);
  gps->println("$PCAS04,7*1E\r\n");
  gps->println("$PCAS02,200*1D\r\n");
#endif

  for (int i = 0; i < 4; ++i)
  {
    satGPSNum[i].begin(GPS, PSTR("GPGSV"), 4 + 4 * i); // offsets 4, 8, 12, 16
    elevGPS[i].begin(GPS, PSTR("GPGSV"), 5 + 4 * i);   // offsets 5, 9, 13, 17
    aziGPS[i].begin(GPS, PSTR("GPGSV"), 6 + 4 * i);    // offsets 6, 10, 14, 18
    snrGPS[i].begin(GPS, PSTR("GPGSV"), 7 + 4 * i);    // offsets 7, 11, 15, 19
  }
}
