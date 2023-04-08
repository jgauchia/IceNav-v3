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
  #define SD_CS 4
  #define SD_MISO 19 
  #define SD_MOSI 23 
  #define SD_CLK 12

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
  #define ADC_BATT_PIN 34

/**
 * @brief BME280 Address
 * 
 */
  #define BME_ADDRESS 0x76
