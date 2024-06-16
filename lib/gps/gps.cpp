/**
 * @file gps.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  GPS definition and functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "gps.hpp"

bool isGpsFixed = false;
bool isTimeFixed = false;
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS = TinyGPSPlus();

#ifdef AT6558D_GPS

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GNGSA"), 15); // $GNGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GNGSA"), 16); // $GNGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GNGSA"), 17); // $GNGSA sentence, 17th element
TinyGPSCustom fixMode(GPS, PSTR("GNGSA"), 2);

GSV GPS_GSV; // GPS Satellites in view
GSV GL_GSV;  // GLONASS Satellites in view
GSV BD_GSV;  // BEIDOU Satellites in view

#else

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GPGSA"), 15); // $GPGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GPGSA"), 16); // $GPGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GPGSA"), 17); // $GPGSA sentence, 17th element
TinyGPSCustom fixMode(GPS, PSTR("GPGSA"), 2);

GSV GPS_GSV; // GPS Satellites in view

#endif

/**
 * @brief Init GPS and custon NMEA parsing
 *
 */
void initGPS()
{
  gps->begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);

#ifdef AT6558D_GPS
  // GPS
  // gps->println("$PCAS04,1*18\r\n");

  // GPS+GLONASS
  // gps->println("$PCAS04,5*1C\r\n");

  // GPS+BDS+GLONASS
  gps->println("$PCAS04,7*1E\r\n");
  gps->flush();
  delay(100);

  gps->println(GPS_RATE_PCAS[gpsUpdate]);
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
  GPS_GSV.satsInView.begin(GPS, PSTR("GPGSV"), 3);

#ifdef AT6558D_GPS

  GL_GSV.totalMsg.begin(GPS, PSTR("GLGSV"), 1);
  GL_GSV.msgNum.begin(GPS, PSTR("GLGSV"), 2);
  GL_GSV.satsInView.begin(GPS, PSTR("GLGSV"), 3);

  BD_GSV.totalMsg.begin(GPS, PSTR("BDGSV"), 1);
  BD_GSV.msgNum.begin(GPS, PSTR("BDGSV"), 2);
  BD_GSV.satsInView.begin(GPS, PSTR("BDGSV"), 3);

#endif

  for (int i = 0; i < 4; ++i)
  {
    GPS_GSV.satNum[i].begin(GPS, PSTR("GPGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    GPS_GSV.elev[i].begin(GPS, PSTR("GPGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    GPS_GSV.azim[i].begin(GPS, PSTR("GPGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    GPS_GSV.snr[i].begin(GPS, PSTR("GPGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

#ifdef AT6558D_GPS

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

/**
 * @brief return latitude from GPS or sys env pre-built variable
 * @return latitude or 0.0 if not defined
 */
double getLat()
{
  if (GPS.location.isValid())
    return GPS.location.lat();
  else
  {
#ifdef DEFAULT_LAT
    return DEFAULT_LAT;
#else
    return 0.0;
#endif
  }
}

/**
 * @brief return longitude from GPS or sys env pre-built variable
 * @return longitude or 0.0 if not defined
 */
double getLon()
{
  if (GPS.location.isValid())
    return GPS.location.lng();
  else
  {
#ifdef DEFAULT_LON
    return DEFAULT_LON;
#else
    return 0.0;
#endif
  }
}
