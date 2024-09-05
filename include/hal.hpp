/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Boards Pin definitions
 * @version 0.1.8_Alpha
 * @date 2024-08
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

  #define ADC_BATT_PIN 34

  extern const uint8_t GPS_TX = 17;
  extern const uint8_t GPS_RX = 18;

  extern const uint8_t TFT_SPI_SCLK = 12;
  extern const uint8_t TFT_SPI_MOSI = 11;
  extern const uint8_t TFT_SPI_MISO = 13;
  extern const uint8_t TFT_SPI_DC   = 3;
  extern const uint8_t TFT_SPI_CS   = 10;
  extern const uint8_t TFT_SPI_RST  = 6;

  extern const uint8_t SD_CS = 21;
  extern const uint8_t SD_MISO = 13;
  extern const uint8_t SD_MOSI = 11;
  extern const uint8_t SD_CLK = 12;

  extern const uint8_t TCH_I2C_PORT = 0;
  extern const uint8_t TCH_I2C_SDA  = 38;
  extern const uint8_t TCH_I2C_SCL  = 39;
  extern const uint8_t TCH_I2C_INT  = 40;
#endif

/**
 * @brief MAKERFABS ESP32S3 BOARD pin definition
 *
 */
#ifdef MAKERF_ESP32S3
  #define LCD_CS 37
  #define LCD_BLK 45

  #define ADC_BATT_PIN 34

  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39

  extern const uint8_t GPS_TX = 17;
  extern const uint8_t GPS_RX = 18;

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
  #define ADC_BATT_PIN 34

  extern const uint8_t GPS_TX = 25;
  extern const uint8_t GPS_RX = 26;

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

  extern const uint8_t SD_CS = 4;
  extern const uint8_t SD_MISO = 19;
  extern const uint8_t SD_MOSI = 23;
  extern const uint8_t SD_CLK = 12;
#endif



/**
 * @brief GPS pin definition
 *
 */
#ifdef ESP32S3_N16R8
  extern const uint8_t GPS_TX = 17;
  extern const uint8_t GPS_RX = 18;
#endif

/**
 * @brief I2C pin definition
 *
 */
#ifdef ESP32S3_N16R8
  #define I2C_SDA_PIN 38
  #define I2C_SCL_PIN 39
#endif

/**
 * @brief Battery monitor pin
 *
 */
#define ADC_BATT_PIN 34

/**
 * @brief TFT Invert color
 *
 */
extern const bool TFT_INVERT = true;

/**
 * @brief TFT SPI pin definition
 *
 */
#ifdef ESP32S3_N16R8
  extern const uint8_t TFT_SPI_SCLK = 12;
  extern const uint8_t TFT_SPI_MOSI = 11;
  extern const uint8_t TFT_SPI_MISO = 13;
  extern const uint8_t TFT_SPI_DC   = 3;
  extern const uint8_t TFT_SPI_CS   = 10;
  extern const uint8_t TFT_SPI_RST  = 6;
#endif

/**
 * @brief TOUCH SPI pin definition
 *
 */
#ifdef ESP32S3_N16R8
  extern const uint8_t TCH_SPI_SCLK = 12;
  extern const uint8_t TCH_SPI_MOSI = 11;
  extern const uint8_t TCH_SPI_MISO = 13;
  extern const uint8_t TCH_SPI_INT  = 5;
  extern const uint8_t TCH_SPI_CS   = 4;
#endif

/**
 * @brief SD Card pin definition
 *
 */
#ifdef ESP32S3_N16R8   
  extern const uint8_t SD_CS = 1;
  extern const uint8_t SD_MISO = 41;
  extern const uint8_t SD_MOSI = 2;
  extern const uint8_t SD_CLK = 42;
#endif

#endif
