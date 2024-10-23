#include <Arduino.h>
#define NMEAGPS_PARSE_GGA
#define NMEAGPS_PARSE_GLL
#define NMEAGPS_PARSE_GSA
#define NMEAGPS_PARSE_GSV
#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
#define NMEAGPS_PARSE_VTG
#define NMEAGPS_PARSE_ZDA
#define NMEAGPS_PARSE_SATELLITES
#define NMEAGPS_PARSE_SATELLITE_INFO

#include <NMEAGPS.h>
#include "settings.hpp"
#include "tasks.hpp"
#include <Streamers.h>

#define DEBUG_PORT Serial
#define gpsPort Serial2
#define GPS_PORT_NAME "Serial2"




NMEAGPS  GPS; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values


#ifndef NMEAGPS_PARSE_GSV
  #error You must define NMEAGPS_PARSE_GSV in NMEAGPS_cfg.h!
#endif

#ifndef NMEAGPS_PARSE_SATELLITES
  #error You must define NMEAGPS_PARSE_SATELLITE in NMEAGPS_cfg.h!
#endif

#ifndef NMEAGPS_PARSE_SATELLITE_INFO
  #error You must define NMEAGPS_PARSE_SATELLITE_INFO in NMEAGPS_cfg.h!
#endif


void displaySatellitesInView()
{
  DEBUG_PORT.print( GPS.sat_count );
  DEBUG_PORT.print( ',' );

  for (uint8_t i=0; i < GPS.sat_count; i++) {
    DEBUG_PORT.print( GPS.satellites[i].id );
    DEBUG_PORT.print( ' ' );
    DEBUG_PORT.print( GPS.satellites[i].elevation );
    DEBUG_PORT.print( '/' );
    DEBUG_PORT.print( GPS.satellites[i].azimuth );
    DEBUG_PORT.print( '@' );
    if (GPS.satellites[i].tracked)
      DEBUG_PORT.print( GPS.satellites[i].snr );
    else
      DEBUG_PORT.print( '-' );
    DEBUG_PORT.print( F(", ") );
  }

  DEBUG_PORT.println();

} // displaySatellitesInView



//-----------------

void setup()
{
  DEBUG_PORT.begin(115200);

  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NeoGPS GSV example started\n") );

  gpsPort.begin( 9600, SERIAL_8N1, 18, 17);

      initCLI();
    initCLITask();

} // setup

//-----------------

void loop()
{
  while (GPS.available( gpsPort )) {
    fix = GPS.read();
       displaySatellitesInView();
  }

} // loop

//-----------------

