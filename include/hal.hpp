/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boards Pin definitions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef HAL_HPP
#define HAL_HPP

/**
 * @brief ICENAV BOARD pin definition
 *
 */
#ifdef ICENAV_BOARD
  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  uint8_t GPS_TX = 43;
  uint8_t GPS_RX = 44;

  extern const uint8_t BOARD_BOOT_PIN = 0;

  extern const uint8_t SD_CS = 1;
  extern const uint8_t SD_MISO = 41;
  extern const uint8_t SD_MOSI = 2;
  extern const uint8_t SD_CLK = 42;

  extern const uint8_t TFT_WR  = 18;
  extern const uint8_t TFT_RD  = 48;
  extern const uint8_t TFT_RS  = 45;
  extern const uint8_t TFT_RST = -1;
  extern const uint8_t TFT_CS  = -1;
  extern const uint8_t TFT_D0  = 47;
  extern const uint8_t TFT_D1  = 21;
  extern const uint8_t TFT_D2  = 14;
  extern const uint8_t TFT_D3  = 13;
  extern const uint8_t TFT_D4  = 12;
  extern const uint8_t TFT_D5  = 11;
  extern const uint8_t TFT_D6  = 10;
  extern const uint8_t TFT_D7  = 9;
  extern const uint8_t TFT_D8  = 3;
  extern const uint8_t TFT_D9  = 8;
  extern const uint8_t TFT_D10 = 16;
  extern const uint8_t TFT_D11 = 15;
  extern const uint8_t TFT_D12 = 7;
  extern const uint8_t TFT_D13 = 6;
  extern const uint8_t TFT_D14 = 5;
  extern const uint8_t TFT_D15 = 4;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = 40;
#endif

/**
 * @brief ELECROW ESP32 Terminal BOARD pin definition
 *
 */
#ifdef ELECROW_ESP32
  #define LCD_CS -1
  #define LCD_BLK 46

  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  // UART PORT
  // uint8_t GPS_TX = 44;  // UART PIN Terminal Port
  // uint8_t GPS_RX = 43;  // UART PIN Terminal Port

  // Alternative to UART PORT
  uint8_t GPS_TX = 40;  // Analog PIN Terminal Port
  uint8_t GPS_RX = 19;  // Digital PIN Terminal Port

  extern const uint8_t BOARD_BOOT_PIN = 0;

  extern const uint8_t SD_CS = 1;
  extern const uint8_t SD_MISO = 41;
  extern const uint8_t SD_MOSI = 2;
  extern const uint8_t SD_CLK = 42;

  extern const uint8_t TFT_WR  = 18;
  extern const uint8_t TFT_RD  = 48;
  extern const uint8_t TFT_RS  = 45;
  extern const uint8_t TFT_RST = -1;
  extern const uint8_t TFT_CS  = -1;
  extern const uint8_t TFT_D0  = 47;
  extern const uint8_t TFT_D1  = 21;
  extern const uint8_t TFT_D2  = 14;
  extern const uint8_t TFT_D3  = 13;
  extern const uint8_t TFT_D4  = 12;
  extern const uint8_t TFT_D5  = 11;
  extern const uint8_t TFT_D6  = 10;
  extern const uint8_t TFT_D7  = 9;
  extern const uint8_t TFT_D8  = 3;
  extern const uint8_t TFT_D9  = 8;
  extern const uint8_t TFT_D10 = 16;
  extern const uint8_t TFT_D11 = 15;
  extern const uint8_t TFT_D12 = 7;
  extern const uint8_t TFT_D13 = 6;
  extern const uint8_t TFT_D14 = 5;
  extern const uint8_t TFT_D15 = 4;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = -1;
#endif

#ifdef TDECK_ESP32S3   // Check GPS UART GPIO
  #define I2C_SDA_PIN 18
  #define I2C_SCL_PIN 8

  #define BOARD_POWERON 10
  extern const uint8_t BOARD_BOOT_PIN = 0;

  uint8_t GPS_TX = 43;
  uint8_t GPS_RX = 44;

  extern const uint8_t TFT_SPI_SCLK = 40;
  extern const uint8_t TFT_SPI_MOSI = 41;
  extern const uint8_t TFT_SPI_MISO = 38;
  extern const uint8_t TFT_SPI_DC   = 11;
  extern const uint8_t TFT_SPI_CS   = 12;
  extern const uint8_t TFT_SPI_RST  = -1;
  extern const uint8_t TFT_SPI_BL   = 42;

  extern const uint8_t SD_CS = 39;
  extern const uint8_t SD_MISO = 38;
  extern const uint8_t SD_MOSI = 41;
  extern const uint8_t SD_CLK = 40;

  extern const uint8_t BOARD_TFT_CS = 12;
  extern const uint8_t RADIO_CS_PIN = 9;

  extern const uint8_t TCH_I2C_PORT = 0;
  extern const uint8_t TCH_I2C_SDA  = 18;
  extern const uint8_t TCH_I2C_SCL  = 8;
  extern const uint8_t TCH_I2C_INT  = 16;
#endif

/**
 * @brief MAKERFABS ESP32S3 BOARD pin definition
 *
 */
#ifdef MAKERF_ESP32S3
  #define LCD_CS 37
  #define LCD_BLK 45

  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  uint8_t GPS_TX = 17;
  uint8_t GPS_RX = 18;

  extern const uint8_t BOARD_BOOT_PIN = 0;

  extern const uint8_t SD_CS = 1;
  extern const uint8_t SD_MISO = 41;
  extern const uint8_t SD_MOSI = 2;
  extern const uint8_t SD_CLK = 42;

  extern const uint8_t TFT_WR  = 35;
  extern const uint8_t TFT_RD  = 48;
  extern const uint8_t TFT_RS  = 36;
  extern const uint8_t TFT_RST = -1;
  extern const uint8_t TFT_CS  = 5;
  extern const uint8_t TFT_D0  = 47;
  extern const uint8_t TFT_D1  = 21;
  extern const uint8_t TFT_D2  = 14;
  extern const uint8_t TFT_D3  = 13;
  extern const uint8_t TFT_D4  = 12;
  extern const uint8_t TFT_D5  = 11;
  extern const uint8_t TFT_D6  = 10;
  extern const uint8_t TFT_D7  = 9;
  extern const uint8_t TFT_D8  = 3;
  extern const uint8_t TFT_D9  = 8;
  extern const uint8_t TFT_D10 = 16;
  extern const uint8_t TFT_D11 = 15;
  extern const uint8_t TFT_D12 = 7;
  extern const uint8_t TFT_D13 = 6;
  extern const uint8_t TFT_D14 = 5;
  extern const uint8_t TFT_D15 = 4;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = 40;
#endif

/**
 * @brief ESP32_N16R4 BOARD pin definition
 *
 */
#ifdef ESP32_N16R4

  uint8_t GPS_TX = 25;
  uint8_t GPS_RX = 26;

  extern const uint8_t BOARD_BOOT_PIN = 0;

  extern const uint8_t TFT_SPI_SCLK = 14;
  extern const uint8_t TFT_SPI_MOSI = 13;
  extern const uint8_t TFT_SPI_MISO = 27;
  extern const uint8_t TFT_SPI_DC   = 15;
  extern const uint8_t TFT_SPI_CS   = 2;
  extern const uint8_t TFT_SPI_RST  = 32;

  extern const uint8_t TCH_SPI_SCLK = 14;
  extern const uint8_t TCH_SPI_MOSI = 13;
  extern const uint8_t TCH_SPI_MISO = 27;
  extern const uint8_t TCH_SPI_INT  = 5;
  extern const uint8_t TCH_SPI_CS   = 18;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = 40;

  extern const uint8_t SD_CS = 4;
  extern const uint8_t SD_MISO = 19;
  extern const uint8_t SD_MOSI = 23;
  extern const uint8_t SD_CLK = 12;
#endif

/**
 * @brief ESP32S3_N16R8 BOARD pin definition
 *
 */
#ifdef ESP32S3_N16R8
  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  uint8_t GPS_TX = 17;
  uint8_t GPS_RX = 18;

  extern const uint8_t BOARD_BOOT_PIN = 0;

  extern const uint8_t TFT_SPI_SCLK = 12;
  extern const uint8_t TFT_SPI_MOSI = 11;
  extern const uint8_t TFT_SPI_MISO = 13;
  extern const uint8_t TFT_SPI_DC   = 3;
  extern const uint8_t TFT_SPI_CS   = 10;
  extern const uint8_t TFT_SPI_RST  = 6;

  extern const uint8_t TCH_SPI_SCLK = 12;
  extern const uint8_t TCH_SPI_MOSI = 11;
  extern const uint8_t TCH_SPI_MISO = 13;
  extern const uint8_t TCH_SPI_INT  = 5;
  extern const uint8_t TCH_SPI_CS   = 4;

  extern const uint8_t TCH_I2C_PORT = 0;  
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = 40;

  extern const uint8_t SD_CS = 1;
  extern const uint8_t SD_MISO = 41;
  extern const uint8_t SD_MOSI = 2;
  extern const uint8_t SD_CLK = 42;
#endif

/**
 * @brief TFT Invert color
 *
 */
extern const bool TFT_INVERT = true;

#endif
