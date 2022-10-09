/**
 * @file hal.h
 * @author Jordi Gauchía
 * @brief  Pin definitions
 * @version 0.1
 * @date 2022-10-09
 */

/**
 * @brief GPS pin definition
 * 
 */
#ifdef CUSTOMBOARD
  #define GPS_TX 16
  #define GPS_RX 17
#endif

#ifdef TDISPLAY
  #define GPS_TX 25
  #define GPS_RX 26
#endif

/**
 * @brief SD pin definition
 * 
 */
#ifdef CUSTOMBOARD
  #define SD_CS 4
  #define SD_MISO 27
  #define SD_MOSI 13
  #define SD_CLK 14
#endif

#ifdef TDISPLAY
  #define SD_CS 2
  #define SD_MISO 27
  #define SD_MOSI 15
  #define SD_CLK 13
#endif

/**
 * @brief Battery monitor pin
 * 
 */
#define ADC_BATT_PIN 34