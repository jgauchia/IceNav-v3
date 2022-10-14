/**
 * @file gps.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.1
 * @date 2022-10-09
 */

#include <TimeLib.h>
#include <TinyGPS++.h>

#define MAX_SATELLITES 60
#define TIME_OFFSET 1
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS;
bool is_gps_fixed = false;

/**
 * @brief Custom NMEA sentences
 *
 */
TinyGPSCustom totalGPGSVMessages(GPS, "GPGSV", 1); // $GPGSV sentence, first element
TinyGPSCustom messageNumber(GPS, "GPGSV", 2);      // $GPGSV sentence, second element
TinyGPSCustom satsInView(GPS, "GPGSV", 3);         // $GPGSV sentence, third element
TinyGPSCustom satNumber[4];                        // to be initialized later
TinyGPSCustom elevation[4];
TinyGPSCustom azimuth[4];
TinyGPSCustom snr[4];

/**
 * @brief Structure for satellite position (elevation, azimut,...)
 *
 */
struct
{
  bool active;
  int elevation;
  int azimuth;
  int snr;
  int pos_x;
  int pos_y;
} sat_tracker[MAX_SATELLITES];

/**
 * @brief Init GPS and custon NMEA parsing
 *
 */
void init_gps()
{
  gps->begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX, GPS_TX);

  for (int i = 0; i < 4; ++i)
  {
    satNumber[i].begin(GPS, "GPGSV", 4 + 4 * i); // offsets 4, 8, 12, 16
    elevation[i].begin(GPS, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
    azimuth[i].begin(GPS, "GPGSV", 6 + 4 * i);   // offsets 6, 10, 14, 18
    snr[i].begin(GPS, "GPGSV", 7 + 4 * i);       // offsets 7, 11, 15, 19
  }
}

/**
 * @brief Get the gps fix signal and set time.
 *
 */
void get_gps_fix()
{
  while (!GPS.location.isValid())
  {
    if (GPS.location.isValid())
    {
      is_gps_fixed = true;
      setTime(GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.date.day(), GPS.date.month(), GPS.date.year());
      delay(50);
      adjustTime(TIME_OFFSET * SECS_PER_HOUR);
      delay(500);
      break;
    }
  }
  lv_timer_handler();
  delay(5);
}