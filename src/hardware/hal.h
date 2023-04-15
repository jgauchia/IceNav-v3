/**
 * @file hal.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Pin definitions
 * @version 0.1.2
 * @date 2023-04-15
 */

#ifdef CUSTOMBOARD
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
#endif

#ifdef MAKERF_ESP32S3  // TODO: we need find the right pins for this board
/**
 * @brief GPS pin definition
 *
 */
  #define GPS_TX 17
  #define GPS_RX 18

  #define LCD_CS 37
  #define LCD_BLK 45

  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  #define SD_MOSI 2
  #define SD_MISO 41
  #define SD_CLK 42
  #define SD_CS 1
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
