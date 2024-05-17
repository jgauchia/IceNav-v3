/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Pin definitions
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef HAL_HPP
#define HAL_HPP

#ifdef CUSTOMBOARD
/**
 * @brief GPS pin definition
 *
 */
extern const uint8_t GPS_TX = 25;
extern const uint8_t GPS_RX = 26;

/**
 * @brief SD pin definition
 *
 */
extern const uint8_t SD_CS = 4;
extern const uint8_t SD_MISO = 19;
extern const uint8_t SD_MOSI = 23;
extern const uint8_t SD_CLK = 12;
#endif

#ifdef MAKERF_ESP32S3 // TODO: we need find the right pins for this board
/**
* @brief GPS pin definition
*
*/
extern const uint8_t GPS_TX = 17;
extern const uint8_t GPS_RX = 18;

#define LCD_CS 37
#define LCD_BLK 45

#define I2C_SDA_PIN 38
#define I2C_SCL_PIN 39

extern const uint8_t SD_CS = 1;
extern const uint8_t SD_MISO = 41;
extern const uint8_t SD_MOSI = 2;
extern const uint8_t SD_CLK = 42;
#endif

/**
 * @brief Battery monitor pin
 *
 */
#define ADC_BATT_PIN 34

#endif