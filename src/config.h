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
 * @brief Define board type
 * 
 * TDISPLAY:  TTGO T-DISPLAY ESP32 MODULE
 * CUSTOMBOARD: ESP32 CUSTOM PCB
 * 
 */
#define TDISPLAY
// #define CUSTOMBOARD

/**
 * @brief Enable compass
 * 
 */
// #define ENABLE_COMPASS

/**
 * @brief Disable WiFi and Bluetooth
 * 
 */
#define DISABLE_RADIO

/**
 * @brief Enable PCF8574 key input
 * 
 */
// #define ENABLE_PCF8574