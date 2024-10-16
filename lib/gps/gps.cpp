/**
 * @file gps.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  GPS definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "gps.hpp"

bool isGpsFixed = false;
bool isTimeFixed = false;
HardwareSerial *gps = &Serial2;
TinyGPSPlus GPS = TinyGPSPlus();
long gpsBaudDetected = 0;
bool nmea_output_enable = false;

// DOP and fix mode
TinyGPSCustom pdop(GPS, PSTR("GNGSA"), 15); // $GNGSA sentence, 15th element
TinyGPSCustom hdop(GPS, PSTR("GNGSA"), 16); // $GNGSA sentence, 16th element
TinyGPSCustom vdop(GPS, PSTR("GNGSA"), 17); // $GNGSA sentence, 17th element
TinyGPSCustom fixMode(GPS, PSTR("GNGSA"), 2);

GSV gnssInfoSV[4]; // GPS Satellites in view

/**
 * @brief Init GPS and custom NMEA parsing
 *
 */
void initGPS()
{
  if (gpsBaud != 4)
    gps->begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
  else
  {
    gpsBaudDetected = autoBaudGPS();

    if (gpsBaudDetected != 0)
    {
      gps->begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
    }
  }
#ifdef AT6558D_GPS
      // GPS
      // gps->println("$PCAS04,1*18\r\n");

      // GPS+GLONASS
      // gps->println("$PCAS04,5*1C\r\n");

   
      // // WARM START¿?
      // gps->println("$PCAS10,0*1C\r\n");
      // gps->flush();
      // delay(100);

      // GPS+BDS+GLONASS
      gps->println("$PCAS04,7*1E\r\n");
      gps->flush();
      delay(100);

      // Update Rate
      gps->println(GPS_RATE_PCAS[gpsUpdate]);
      gps->flush();
      delay(100);

      // Set NMEA 4.1
      gps->println("$PCAS05,2*1A\r\n");
      gps->flush();
      delay(100);
#endif

  // Initialize satellites in view custom NMEA structure
  gnssInfoSV[0].totalMsg.begin(GPS, PSTR("GPGSV"), 1);
  gnssInfoSV[0].msgNum.begin(GPS, PSTR("GPGSV"), 2);
  gnssInfoSV[0].satsInView.begin(GPS, PSTR("GPGSV"), 3);

  gnssInfoSV[1].totalMsg.begin(GPS, PSTR("GLGSV"), 1);
  gnssInfoSV[1].msgNum.begin(GPS, PSTR("GLGSV"), 2);
  gnssInfoSV[1].satsInView.begin(GPS, PSTR("GLGSV"), 3);

  gnssInfoSV[2].totalMsg.begin(GPS, PSTR("BDGSV"), 1);
  gnssInfoSV[2].msgNum.begin(GPS, PSTR("BDGSV"), 2);
  gnssInfoSV[2].satsInView.begin(GPS, PSTR("BDGSV"), 3);


  for (int i = 0; i < 4; ++i)
  {
    gnssInfoSV[0].satNum[i].begin(GPS, PSTR("GPGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    gnssInfoSV[0].elev[i].begin(GPS, PSTR("GPGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    gnssInfoSV[0].azim[i].begin(GPS, PSTR("GPGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    gnssInfoSV[0].snr[i].begin(GPS, PSTR("GPGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

    gnssInfoSV[1].satNum[i].begin(GPS, PSTR("GLGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    gnssInfoSV[1].elev[i].begin(GPS, PSTR("GLGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    gnssInfoSV[1].azim[i].begin(GPS, PSTR("GLGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    gnssInfoSV[1].snr[i].begin(GPS, PSTR("GLGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19

    gnssInfoSV[2].satNum[i].begin(GPS, PSTR("BDGSV"), 4 + (4 * i)); // offsets 4, 8, 12, 16
    gnssInfoSV[2].elev[i].begin(GPS, PSTR("BDGSV"), 5 + (4 * i));   // offsets 5, 9, 13, 17
    gnssInfoSV[2].azim[i].begin(GPS, PSTR("BDGSV"), 6 + (4 * i));   // offsets 6, 10, 14, 18
    gnssInfoSV[2].snr[i].begin(GPS, PSTR("BDGSV"), 7 + (4 * i));    // offsets 7, 11, 15, 19
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
  else if (cfg.getDouble(PKEYS::KLAT_DFL,0.0) != 0.0)
  {
    //log_v("getLat: %02f",cfg.getDouble(PKEYS::KLAT_DFL,0.0));
    return cfg.getDouble(PKEYS::KLAT_DFL,0.0);
  }
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
  else if (cfg.getDouble(PKEYS::KLON_DFL,0.0) != 0.0)
  {
    //log_v("getLon: %02f",cfg.getDouble(PKEYS::KLON_DFL,0.0));
    return cfg.getDouble(PKEYS::KLON_DFL,0.0);
  }
  else
  {
#ifdef DEFAULT_LON
    return DEFAULT_LON;
#else
    return 0.0;
#endif
  }
}

/**
 *  @brief return pulse rate from RX GPS pin
 *  @return pulse rate
 */
long detectRate(int rxPin)  
{
  long  rate = 10000, x=2000;
  pinMode(rxPin, INPUT);      // make sure serial in is a input pin
  digitalWrite (rxPin, HIGH); // pull up enabled just for noise protection

  for (int i = 0; i < 5; i++)
  {
    x = pulseIn(rxPin,LOW, 125000);   // measure the next zero bit width
    if(x<1)continue;
      rate = x < rate ? x : rate;
  }
  return rate;
}

/**
 *  @brief Detect GPS Baudrate
 *  @return baudrate
 */
long autoBaudGPS()
{
	long rate = detectRate(GPS_RX)+detectRate(GPS_RX)+detectRate(GPS_RX);
	rate = rate/3l;
	long baud = 0;
	/*
	 	 Time	Baud Rate
		3333µs (3.3ms)300
		833µs 	1200
		416µs 	2400
		208µs 	4800
		104µs 	9600
		69µs 	14400
		52µs 	19200
		34µs 	28800
		26µs 	38400
		17.3µs 	57600
		8µs 	115200
		Megas min is about 10uS? so there may be some inaccuracy
	 */
	  if (rate < 12)
      baud = 115200;
    else if (rate < 20)
      baud = 57600;
    else if (rate < 30)
      baud = 38400;
    else if (rate < 40)
      baud = 28800;
    else if (rate < 60)
      baud = 19200;
    else if (rate < 80)
      baud = 14400;
    else if (rate < 150)
      baud = 9600;
    else if (rate < 300)
      baud = 4800;
    else if (rate < 600)
      baud = 2400;
    else if (rate < 1200)
      baud = 1200;
    else
      baud = 0;

		return baud;
}
