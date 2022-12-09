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
  #define SD_MISO 27
  #define SD_MOSI 13
  #define SD_CLK 14
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

/**
 * @brief Battery monitor pin
 *
 */
#ifdef CUSTOMBOARD
  #define ADC_BATT_PIN 34
#endif