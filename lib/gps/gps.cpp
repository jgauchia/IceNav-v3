/**
 * @file gps.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  GPS definition and functions
 * @version 0.1.9
 * @date 2024-12
 */

#include "gps.hpp"

bool isGpsFixed = false;
bool isTimeFixed = false;
long gpsBaudDetected = 0;
bool nmea_output_enable = false;
gps_fix  fix;
NMEAGPS  GPS;
GPSDATA gpsData;
SV satTracker[MAX_SATELLITES];
NeoGPS::time_t localTime = 0;

/**
 * @brief Init GPS and custom NMEA parsing
 *
 */
void initGPS()
{
  gpsPort.setRxBufferSize(1024);

  if (gpsBaud != 4)
    gpsPort.begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
  else
  {
    gpsBaudDetected = autoBaudGPS();

    if (gpsBaudDetected != 0)
    {
      gpsPort.begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
    }
  }

#ifdef AT6558D_GPS
      // FACTORY RESET
      // gpsPort.println("$PCAS10,3*1F\r\n");
      // gpsPort.flush();
      // delay(100);

      // GPS
      // gpsPort.println("$PCAS04,1*18\r\n")

      // GPS+GLONASS
      // gpsPort.println("$PCAS04,5*1C\r\n");

      // GPS+BDS+GLONASS
      gpsPort.println("$PCAS04,7*1E\r\n");
      gpsPort.flush();
      delay(100);

      // Update Rate
      gpsPort.println(GPS_RATE_PCAS[gpsUpdate]);
      gpsPort.flush();
      delay(100);

      // Set NMEA 4.1 
      gpsPort.println("$PCAS05,2*1A\r\n");
      gpsPort.flush();
      delay(100);
#endif

    localTime.init();
}

/**
 * @brief return latitude from GPS or sys env pre-built variable
 * @return latitude or 0.0 if not defined
 */
double getLat()
{
  if (fix.valid.location)
    return fix.latitude();
  else if (cfg.getDouble(PKEYS::KLAT_DFL,0.0) != 0.0)
  {
    // log_v("getLat: %02f",cfg.getDouble(PKEYS::KLAT_DFL,0.0));
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
  if (fix.valid.location)
    return fix.longitude();
  else if (cfg.getDouble(PKEYS::KLON_DFL,0.0) != 0.0)
  {
    // log_v("getLon: %02f",cfg.getDouble(PKEYS::KLON_DFL,0.0));
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
 * @brief Get GPS parsed data
 *
 */
void getGPSData()
{
  // GPS Fix
  if (fix.status != gps_fix::STATUS_NONE)
    isGpsFixed = true;

  // GPS Not fixed
  if (fix.status == gps_fix::STATUS_NONE)
    isGpsFixed = false;

  // Satellite Count
  gpsData.satellites = fix.satellites;

  // Fix Mode
  gpsData.fixMode = fix.status;

  // Time and Date
  if (fix.valid.time && fix.valid.date)
  {
    adjustTime( fix.dateTime );   
    localTime = fix.dateTime;
    if (calcSun)
    {
      // Calculate Sunrise and Sunset only one time when date & time was valid
      calculateSun();
      log_v("Get date, time, Sunrise and Sunset");  
      calcSun = false;
    }
  }

  // Altitude
  if (fix.valid.altitude)
    gpsData.altitude = fix.alt.whole;

  // Speed
  if (fix.valid.speed)
    gpsData.speed = (uint16_t)fix.speed_kph();

  // Latitude and Longitude
  if (fix.valid.location)
    gpsData.latitude = getLat();
    gpsData.longitude = getLon();

  // Heading
  if (fix.valid.heading)
    gpsData.heading = (uint16_t)fix.heading();

  // HDOP , PDOP , VDOP
  if (fix.valid.hdop)
    gpsData.hdop = (float)fix.hdop / 1000;
  if (fix.valid.pdop)
    gpsData.pdop = (float)fix.pdop / 1000;
  if (fix.valid.vdop)
    gpsData.vdop = (float)fix.vdop / 1000;

  // Satellite info
  gpsData.satInView = (uint8_t)GPS.sat_count;
  for (uint8_t i = 0; i < gpsData.satInView; i++) 
  {
    satTracker[i].satNum = (uint8_t)GPS.satellites[i].id;
    satTracker[i].elev = (uint8_t)GPS.satellites[i].elevation;
    satTracker[i].azim = (uint16_t)GPS.satellites[i].azimuth;
    satTracker[i].snr = (uint8_t)GPS.satellites[i].snr;
    satTracker[i].active = GPS.satellites[i].tracked;
    strncpy(satTracker[i].talker_id, GPS.satellites[i].talker_id, 3);
    int H = canvasRadius * (90 - satTracker[i].elev) / 90;  
    satTracker[i].posX = canvasCenter_X + H * sin(DEG2RAD(satTracker[i].azim));
    satTracker[i].posY = canvasCenter_Y - H * cos(DEG2RAD(satTracker[i].azim)); 
  }
}

/**
 *  @brief return pulse rate from RX GPS pin
 *  @return pulse rate
 */
long detectRate(int rxPin)  
{
  long  rate = 10000, x=2000;
  pinMode(rxPin, INPUT);      // make sure Serial in is a input pin
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
