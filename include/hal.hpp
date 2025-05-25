/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boards Pin definitions
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

/**
 * @brief ICENAV BOARD pin definition
 *
 */
#ifdef ICENAV_BOARD
  #define I2C_SDA_PIN   GPIO_NUM_38
  #define I2C_SCL_PIN   GPIO_NUM_39

  uint8_t GPS_TX = GPIO_NUM_43;
  uint8_t GPS_RX = GPIO_NUM_44;

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  extern const uint8_t SD_CS = GPIO_NUM_1;
  extern const uint8_t SD_MISO = GPIO_NUM_41;
  extern const uint8_t SD_MOSI = GPIO_NUM_2;
  extern const uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
* @brief LilyGO T-DECK BOARD pin definition
*
*/
#ifdef TDECK_ESP32S3   
  #define I2C_SDA_PIN   GPIO_NUM_18
  #define I2C_SCL_PIN   GPIO_NUM_8
  #define BOARD_POWERON GPIO_NUM_10

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  uint8_t GPS_TX = GPIO_NUM_43;
  uint8_t GPS_RX = GPIO_NUM_44;

  extern const uint8_t TFT_SPI_CS   = GPIO_NUM_12;
  extern const uint8_t RADIO_CS_PIN = GPIO_NUM_9;
  extern const uint8_t SPI_MISO = GPIO_NUM_38;

  extern const uint8_t SD_CS = GPIO_NUM_39;
  extern const uint8_t SD_MISO = GPIO_NUM_38;
  extern const uint8_t SD_MOSI = GPIO_NUM_41;
  extern const uint8_t SD_CLK = GPIO_NUM_40;
#endif

/**
 * @brief ELECROW ESP32 Terminal BOARD pin definition
 *
 */
#ifdef ELECROW_ESP32
  #define I2C_SDA_PIN   GPIO_NUM_38
  #define I2C_SCL_PIN   GPIO_NUM_39

  // UART PORT
  // uint8_t GPS_TX = GPIO_NUM_44;  // UART PIN Terminal Port
  // uint8_t GPS_RX = GPIO_NUM_43;  // UART PIN Terminal Port
  // Alternative to UART PORT
  uint8_t GPS_TX = GPIO_NUM_40;  // Analog PIN Terminal Port
  uint8_t GPS_RX = GPIO_NUM_19;  // Digital PIN Terminal Port

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  extern const uint8_t SD_CS = GPIO_NUM_1;
  extern const uint8_t SD_MISO = GPIO_NUM_41;
  extern const uint8_t SD_MOSI = GPIO_NUM_2;
  extern const uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
 * @brief MAKERFABS ESP32S3 BOARD pin definition
 *
 */
#ifdef MAKERF_ESP32S3
  #define I2C_SDA_PIN   GPIO_NUM_38
  #define I2C_SCL_PIN   GPIO_NUM_39

  uint8_t GPS_TX = GPIO_NUM_17;
  uint8_t GPS_RX = GPIO_NUM_18;

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  extern const uint8_t SD_CS = GPIO_NUM_1;
  extern const uint8_t SD_MISO = GPIO_NUM_41;
  extern const uint8_t SD_MOSI = GPIO_NUM_2;
  extern const uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
 * @brief ESP32_N16R4 BOARD pin definition
 *
 */
#ifdef ESP32_N16R4
  #define I2C_SDA_PIN   GPIO_NUM_38
  #define I2C_SCL_PIN   GPIO_NUM_39

  uint8_t GPS_TX = GPIO_NUM_25;
  uint8_t GPS_RX = GPIO_NUM_26;

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  extern const uint8_t TFT_SPI_SCLK = GPIO_NUM_14;
  extern const uint8_t TFT_SPI_MOSI = GPIO_NUM_13;
  extern const uint8_t TFT_SPI_MISO = GPIO_NUM_27;
  extern const uint8_t TFT_SPI_DC   = GPIO_NUM_15;
  extern const uint8_t TFT_SPI_CS   = GPIO_NUM_2;
  extern const uint8_t TFT_SPI_RST  = GPIO_NUM_32;

  extern const uint8_t TCH_SPI_SCLK = GPIO_NUM_14;
  extern const uint8_t TCH_SPI_MOSI = GPIO_NUM_13;
  extern const uint8_t TCH_SPI_MISO = GPIO_NUM_27;
  extern const uint8_t TCH_SPI_INT  = GPIO_NUM_5;
  extern const uint8_t TCH_SPI_CS   = GPIO_NUM_18;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = GPIO_NUM_38;
  extern const uint8_t TCH_I2C_SCL  = GPIO_NUM_39;
  extern const uint8_t TCH_I2C_INT  = GPIO_NUM_40;

  extern const uint8_t SD_CS = GPIO_NUM_4;
  extern const uint8_t SD_MISO = GPIO_NUM_19;
  extern const uint8_t SD_MOSI = GPIO_NUM_23;
  extern const uint8_t SD_CLK = GPIO_NUM_12;
#endif

/**
 * @brief ESP32S3_N16R8 BOARD pin definition
 *
 */
#ifdef ESP32S3_N16R8
  #define I2C_SDA_PIN   GPIO_NUM_38
  #define I2C_SCL_PIN   GPIO_NUM_39

  uint8_t GPS_TX = GPIO_NUM_17;
  uint8_t GPS_RX = GPIO_NUM_18;

  extern const uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

  extern const uint8_t TFT_SPI_SCLK = GPIO_NUM_12;
  extern const uint8_t TFT_SPI_MOSI = GPIO_NUM_11;
  extern const uint8_t TFT_SPI_MISO = GPIO_NUM_13;
  extern const uint8_t TFT_SPI_DC   = GPIO_NUM_3;
  extern const uint8_t TFT_SPI_CS   = GPIO_NUM_10;
  extern const uint8_t TFT_SPI_RST  = GPIO_NUM_6;

  extern const uint8_t TCH_SPI_SCLK = GPIO_NUM_12;
  extern const uint8_t TCH_SPI_MOSI = GPIO_NUM_11;
  extern const uint8_t TCH_SPI_MISO = GPIO_NUM_13;
  extern const uint8_t TCH_SPI_INT  = GPIO_NUM_5;
  extern const uint8_t TCH_SPI_CS   = GPIO_NUM_4;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = GPIO_NUM_38;
  extern const uint8_t TCH_I2C_SCL  = GPIO_NUM_39;
  extern const uint8_t TCH_I2C_INT  = GPIO_NUM_40;

  extern const uint8_t SD_CS = GPIO_NUM_21;
  extern const uint8_t SD_MISO = GPIO_NUM_13;
  extern const uint8_t SD_MOSI = GPIO_NUM_11;
  extern const uint8_t SD_CLK = GPIO_NUM_12;
#endif

/**
 * @brief TFT Invert color
 *
 */
extern const bool TFT_INVERT = true;


