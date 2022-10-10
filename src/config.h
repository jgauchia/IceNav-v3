/**
 * @file config.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Device configuration and debug
 * @version 0.1
 * @date 2022-10-09
 */

/**
 * @brief Enable serial port debug output
 *  
 */
#define DEBUG 

/**
 * @brief serial port baudrate
 * 
 */
#define BAUDRATE 115200

/**
 * @brief GPS baudrate
 * 
 */
#define GPS_BAUDRATE 9600

/**
 * @brief Enable serial port NMEA sentences output 
 * 
 */
//#define OUTPUT_NMEA 

/**
 * @brief Enable searching satellite screen on boot
 * 
 */
//#define SEARCH_SAT_ON_INIT 

/**
 * @brief Sprite buffer
 * 
 */
#define USE_LINE_BUFFER

/**
 * @brief Disable WiFi and Bluetooth
 * 
 */
#define DISABLE_RADIO
