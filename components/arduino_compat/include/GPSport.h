/**
 * @file GPSport.h
 * @brief GPS port configuration for ESP-IDF
 *
 * This replaces NeoGPS's GPSport.h for ESP-IDF compatibility.
 * Uses Serial1 (UART1) for GPS communication.
 */

#ifndef GPSport_h
#define GPSport_h

#include "HardwareSerial.h"

// Use Serial1 for GPS device (UART1)
#define gpsPort Serial1
#define GPS_PORT_NAME "Serial1"

// Use Serial for debug output (UART0 / USB)
#define DEBUG_PORT Serial

#endif // GPSport_h
