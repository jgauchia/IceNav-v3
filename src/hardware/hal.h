/**
 * @file hal.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Pin definitions
 * @version 0.1
 * @date 2022-10-09
 */

/**
 * @brief GPS pin definition
 *
 */
  #define GPS_TX 25
  #define GPS_RX 26

/**
 * @brief SD pin definition
 *
 */
#ifdef CUSTOMBOARD
  #define SD_CS 4
  #define SD_MISO 19 
  #define SD_MOSI 23 
  #define SD_CLK 12
#endif

#ifdef TDISPLAY
  #define SD_CS 2
  #define SD_MISO 27
  #define SD_MOSI 15
  #define SD_CLK 13
  #define ADC_EN 14
  #define BUTTON_R 35
  #define BUTTON_L 0
  #define HW_EN 33
#endif

#ifdef MAKERF_ESP32S3  // TODO: we need find the right pins for this board
  #define SD_CS 4
  #define SD_MISO 19 
  #define SD_MOSI 23 
  #define SD_CLK 12
#endif

/**
 * @brief Battery monitor pin
 *
 */
#ifdef CUSTOMBOARD
  #define ADC_BATT_PIN 34
#endif

/**
 * @brief BME280 Address
 * 
 */
#ifdef CUSTOMBOARD
  #define BME_ADDRESS 0x76
#endif