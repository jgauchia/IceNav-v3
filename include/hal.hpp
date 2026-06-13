/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boards Pin definitions
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

/**
 * @brief ICENAV BOARD pin definition
 *
 */
#ifdef ICENAV_BOARD
    #define I2C_SDA_PIN   GPIO_NUM_38
    #define I2C_SCL_PIN   GPIO_NUM_39

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_43;
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_44;

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t SD_CS = GPIO_NUM_1;
    static constexpr uint8_t SD_MISO = GPIO_NUM_41;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_2;
    static constexpr uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
* @brief LilyGO T-DECK BOARD pin definition
*
*/
#ifdef TDECK_ESP32S3
    #define I2C_SDA_PIN   GPIO_NUM_18
    #define I2C_SCL_PIN   GPIO_NUM_8
    #define BOARD_POWERON GPIO_NUM_10

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_43;
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_44;

    static constexpr uint8_t TFT_SPI_CS   = GPIO_NUM_12;
    static constexpr uint8_t RADIO_CS_PIN = GPIO_NUM_9;
    static constexpr uint8_t SPI_MISO = GPIO_NUM_38;

    static constexpr uint8_t SD_CS = GPIO_NUM_39;
    static constexpr uint8_t SD_MISO = GPIO_NUM_38;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_41;
    static constexpr uint8_t SD_CLK = GPIO_NUM_40;
#endif

/**
 * @brief ELECROW ESP32 Terminal BOARD pin definition
 *
 */
#ifdef ELECROW_ESP32
    #define I2C_SDA_PIN   GPIO_NUM_38
    #define I2C_SCL_PIN   GPIO_NUM_39

    // UART PORT alternative: GPIO_NUM_44 TX / GPIO_NUM_43 RX
    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_40;  // Analog PIN Terminal Port
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_19;  // Digital PIN Terminal Port

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t SD_CS = GPIO_NUM_1;
    static constexpr uint8_t SD_MISO = GPIO_NUM_41;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_2;
    static constexpr uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
 * @brief MAKERFABS ESP32S3 BOARD pin definition
 *
 */
#ifdef MAKERF_ESP32S3
    #define I2C_SDA_PIN   GPIO_NUM_38
    #define I2C_SCL_PIN   GPIO_NUM_39

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_17;
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_18;

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t SD_CS = GPIO_NUM_1;
    static constexpr uint8_t SD_MISO = GPIO_NUM_41;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_2;
    static constexpr uint8_t SD_CLK = GPIO_NUM_42;
#endif

/**
 * @brief ESP32S3_N16R8 BOARD pin definition
 *
 */
#ifdef ESP32S3_N16R8
    #define I2C_SDA_PIN   GPIO_NUM_38
    #define I2C_SCL_PIN   GPIO_NUM_39

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_17;
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_18;

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t TFT_SPI_SCLK = GPIO_NUM_12;
    static constexpr uint8_t TFT_SPI_MOSI = GPIO_NUM_11;
    static constexpr uint8_t TFT_SPI_MISO = GPIO_NUM_13;
    static constexpr uint8_t TFT_SPI_DC   = GPIO_NUM_3;
    static constexpr uint8_t TFT_SPI_CS   = GPIO_NUM_10;
    static constexpr uint8_t TFT_SPI_RST  = GPIO_NUM_6;

    static constexpr uint8_t TCH_SPI_SCLK = GPIO_NUM_12;
    static constexpr uint8_t TCH_SPI_MOSI = GPIO_NUM_11;
    static constexpr uint8_t TCH_SPI_MISO = GPIO_NUM_13;
    static constexpr uint8_t TCH_SPI_INT  = GPIO_NUM_5;
    static constexpr uint8_t TCH_SPI_CS   = GPIO_NUM_4;

    static constexpr uint8_t TCH_I2C_PORT = 0;
    static constexpr uint8_t TCH_I2C_SDA  = GPIO_NUM_38;
    static constexpr uint8_t TCH_I2C_SCL  = GPIO_NUM_39;
    static constexpr uint8_t TCH_I2C_INT  = GPIO_NUM_40;

    static constexpr uint8_t SD_CS = GPIO_NUM_21;
    static constexpr uint8_t SD_MISO = GPIO_NUM_13;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_11;
    static constexpr uint8_t SD_CLK = GPIO_NUM_12;
#endif

#ifdef T4_S3
    #define I2C_SDA_PIN GPIO_NUM_6
    #define I2C_SCL_PIN GPIO_NUM_7
    static constexpr uint8_t TCH_I2C_INT  = GPIO_NUM_8;

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_43;
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_44;

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_0;

    static constexpr uint8_t SD_CS = GPIO_NUM_1;
    static constexpr uint8_t SD_MISO = GPIO_NUM_4;
    static constexpr uint8_t SD_MOSI = GPIO_NUM_2;
    static constexpr uint8_t SD_CLK = GPIO_NUM_3;
#endif

/**
 * @brief TFT Invert color
 *
 */
static constexpr bool TFT_INVERT = true;

void initHAL();

