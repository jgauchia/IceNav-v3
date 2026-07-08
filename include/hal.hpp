/**
 * @file hal.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boards Pin definitions
 * @version 0.3.0
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

    static constexpr uint8_t TFT_WR  = GPIO_NUM_18;
    static constexpr uint8_t TFT_RD  = GPIO_NUM_48;
    static constexpr uint8_t TFT_RS  = GPIO_NUM_45;
    static constexpr uint8_t TFT_D0  = GPIO_NUM_47;
    static constexpr uint8_t TFT_D1  = GPIO_NUM_21;
    static constexpr uint8_t TFT_D2  = GPIO_NUM_14;
    static constexpr uint8_t TFT_D3  = GPIO_NUM_13;
    static constexpr uint8_t TFT_D4  = GPIO_NUM_12;
    static constexpr uint8_t TFT_D5  = GPIO_NUM_11;
    static constexpr uint8_t TFT_D6  = GPIO_NUM_10;
    static constexpr uint8_t TFT_D7  = GPIO_NUM_9;
    static constexpr uint8_t TFT_D8  = GPIO_NUM_3;
    static constexpr uint8_t TFT_D9  = GPIO_NUM_8;
    static constexpr uint8_t TFT_D10 = GPIO_NUM_16;
    static constexpr uint8_t TFT_D11 = GPIO_NUM_15;
    static constexpr uint8_t TFT_D12 = GPIO_NUM_7;
    static constexpr uint8_t TFT_D13 = GPIO_NUM_6;
    static constexpr uint8_t TFT_D14 = GPIO_NUM_5;
    static constexpr uint8_t TFT_D15 = GPIO_NUM_4;
    static constexpr uint8_t TFT_BL  = GPIO_NUM_46;

    static constexpr uint8_t TCH_I2C_INT = GPIO_NUM_40;
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

    static constexpr uint8_t TFT_SPI_SCLK = GPIO_NUM_40;
    static constexpr uint8_t TFT_SPI_MOSI = GPIO_NUM_41;
    static constexpr uint8_t TFT_SPI_MISO = GPIO_NUM_38;
    static constexpr uint8_t TFT_SPI_DC   = GPIO_NUM_11;
    static constexpr uint8_t TFT_BL       = GPIO_NUM_42;

    static constexpr uint8_t TCH_I2C_INT = GPIO_NUM_16;
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

    static constexpr uint8_t TFT_WR  = GPIO_NUM_18;
    static constexpr uint8_t TFT_RD  = GPIO_NUM_48;
    static constexpr uint8_t TFT_RS  = GPIO_NUM_45;
    static constexpr uint8_t TFT_D0  = GPIO_NUM_47;
    static constexpr uint8_t TFT_D1  = GPIO_NUM_21;
    static constexpr uint8_t TFT_D2  = GPIO_NUM_14;
    static constexpr uint8_t TFT_D3  = GPIO_NUM_13;
    static constexpr uint8_t TFT_D4  = GPIO_NUM_12;
    static constexpr uint8_t TFT_D5  = GPIO_NUM_11;
    static constexpr uint8_t TFT_D6  = GPIO_NUM_10;
    static constexpr uint8_t TFT_D7  = GPIO_NUM_9;
    static constexpr uint8_t TFT_D8  = GPIO_NUM_3;
    static constexpr uint8_t TFT_D9  = GPIO_NUM_8;
    static constexpr uint8_t TFT_D10 = GPIO_NUM_16;
    static constexpr uint8_t TFT_D11 = GPIO_NUM_15;
    static constexpr uint8_t TFT_D12 = GPIO_NUM_7;
    static constexpr uint8_t TFT_D13 = GPIO_NUM_6;
    static constexpr uint8_t TFT_D14 = GPIO_NUM_5;
    static constexpr uint8_t TFT_D15 = GPIO_NUM_4;
    static constexpr uint8_t TFT_BL  = GPIO_NUM_46;
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

    static constexpr uint8_t TFT_WR  = GPIO_NUM_35;
    static constexpr uint8_t TFT_RD  = GPIO_NUM_48;
    static constexpr uint8_t TFT_RS  = GPIO_NUM_36;
    static constexpr uint8_t TFT_D0  = GPIO_NUM_47;
    static constexpr uint8_t TFT_D1  = GPIO_NUM_21;
    static constexpr uint8_t TFT_D2  = GPIO_NUM_14;
    static constexpr uint8_t TFT_D3  = GPIO_NUM_13;
    static constexpr uint8_t TFT_D4  = GPIO_NUM_12;
    static constexpr uint8_t TFT_D5  = GPIO_NUM_11;
    static constexpr uint8_t TFT_D6  = GPIO_NUM_10;
    static constexpr uint8_t TFT_D7  = GPIO_NUM_9;
    static constexpr uint8_t TFT_D8  = GPIO_NUM_3;
    static constexpr uint8_t TFT_D9  = GPIO_NUM_8;
    static constexpr uint8_t TFT_D10 = GPIO_NUM_16;
    static constexpr uint8_t TFT_D11 = GPIO_NUM_15;
    static constexpr uint8_t TFT_D12 = GPIO_NUM_7;
    static constexpr uint8_t TFT_D13 = GPIO_NUM_6;
    static constexpr uint8_t TFT_D14 = GPIO_NUM_5;
    static constexpr uint8_t TFT_D15 = GPIO_NUM_4;
    static constexpr uint8_t TFT_CS  = GPIO_NUM_37;
    static constexpr uint8_t TFT_BL  = GPIO_NUM_45;

    static constexpr uint8_t TCH_I2C_INT = GPIO_NUM_40;
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
    static constexpr uint8_t TFT_BL       = GPIO_NUM_45;

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
 * @brief Waveshare ESP32-P4-WIFI6-Touch-LCD-3.5 pin definition
 *
 * @details Display/touch/I2C/SD/power-button/battery (AXP2101 PMIC, shared
 *          I2C bus) pins verified against the Waveshare BSP and schematic
 *          (esp32_p4_wifi6_touch_lcd_35). GPS UART still TODO.
 */
#ifdef WAVESHARE_P4_35
    #define I2C_SDA_PIN   GPIO_NUM_7
    #define I2C_SCL_PIN   GPIO_NUM_8

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_34;  // TODO: confirmar UART libre
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_36;  // TODO: confirmar UART libre

    // Power button: PWR_KEY, wired to the AXP2101 PMIC (not the P4 strapping
    // BOOT pin). Verified against the schematic (PMIC block).
    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_49;

    // ST7796 over SPI (no MISO on this board)
    static constexpr uint8_t TFT_SCLK = GPIO_NUM_21;
    static constexpr uint8_t TFT_MOSI = GPIO_NUM_20;
    static constexpr uint8_t TFT_CS   = GPIO_NUM_23;
    static constexpr uint8_t TFT_DC   = GPIO_NUM_26;
    static constexpr uint8_t TFT_RST  = GPIO_NUM_27;
    static constexpr uint8_t TFT_BL   = GPIO_NUM_28;

    // FT5x06/FT6336 capacitive touch (I2C shared bus)
    static constexpr uint8_t TCH_I2C_INT = GPIO_NUM_50;
    static constexpr uint8_t TCH_I2C_RST = GPIO_NUM_29;

    // microSD over SDIO, SDMMC slot 0, 4-bit (SDMMC pendiente de implementar)
    static constexpr uint8_t SD_CLK_SDIO = GPIO_NUM_43;
    static constexpr uint8_t SD_CMD      = GPIO_NUM_44;
    static constexpr uint8_t SD_D0       = GPIO_NUM_39;
    static constexpr uint8_t SD_D1       = GPIO_NUM_40;
    static constexpr uint8_t SD_D2       = GPIO_NUM_41;
    static constexpr uint8_t SD_D3       = GPIO_NUM_42;

    // ESP32-C6FH8 co-processor (WiFi via esp-hosted), SDIO bus
    // Verified against the Waveshare P4-3.5 schematic (ESP32-C6FH8 block).
    static constexpr uint8_t C6_SDIO_CLK = GPIO_NUM_19;
    static constexpr uint8_t C6_SDIO_CMD = GPIO_NUM_18;
    static constexpr uint8_t C6_SDIO_D0  = GPIO_NUM_14;
    static constexpr uint8_t C6_SDIO_D1  = GPIO_NUM_15;
    static constexpr uint8_t C6_SDIO_D2  = GPIO_NUM_16;
    static constexpr uint8_t C6_SDIO_D3  = GPIO_NUM_17;
    static constexpr uint8_t C6_SDIO_RST = GPIO_NUM_54;
#endif

/**
 * @brief Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 pin definition
 *
 * @details Provisional pins. TODO: verificar contra el esquemático real.
 *          Display is MIPI-DSI; only the shared buses are sketched here.
 */
#ifdef WAVESHARE_P4_43
    #define I2C_SDA_PIN   GPIO_NUM_7   // TODO: verificar (esquemático real)
    #define I2C_SCL_PIN   GPIO_NUM_8   // TODO: verificar (esquemático real)

    static constexpr uint8_t GPS_TX_DEFAULT = GPIO_NUM_37;  // TODO: verificar (esquemático real)
    static constexpr uint8_t GPS_RX_DEFAULT = GPIO_NUM_38;  // TODO: verificar (esquemático real)

    static constexpr uint8_t BOARD_BOOT_PIN = GPIO_NUM_35;  // TODO: verificar (esquemático real)

    static constexpr uint8_t TCH_I2C_INT = GPIO_NUM_14;  // TODO: verificar (esquemático real)

    // Placeholder SPI pins only to satisfy the temporary LovyanGFX panel header.
    // The real display is MIPI-DSI (esp_lcd backend), which replaces these.
    static constexpr uint8_t TFT_SCLK = GPIO_NUM_5;   // placeholder
    static constexpr uint8_t TFT_MOSI = GPIO_NUM_6;   // placeholder
    static constexpr uint8_t TFT_CS   = GPIO_NUM_10;  // placeholder
    static constexpr uint8_t TFT_DC   = GPIO_NUM_11;  // placeholder
    static constexpr uint8_t TFT_RST  = GPIO_NUM_12;  // placeholder
    static constexpr uint8_t TFT_BL   = GPIO_NUM_13;  // placeholder

    // microSD over SDIO. TODO: verificar (esquemático real; SDMMC pendiente)
    static constexpr uint8_t SD_CLK_SDIO = GPIO_NUM_43;  // TODO: verificar (esquemático real)
    static constexpr uint8_t SD_CMD      = GPIO_NUM_44;  // TODO: verificar (esquemático real)
    static constexpr uint8_t SD_D0       = GPIO_NUM_39;  // TODO: verificar (esquemático real)
    static constexpr uint8_t SD_D1       = GPIO_NUM_40;  // TODO: verificar (esquemático real)
    static constexpr uint8_t SD_D2       = GPIO_NUM_41;  // TODO: verificar (esquemático real)
    static constexpr uint8_t SD_D3       = GPIO_NUM_42;  // TODO: verificar (esquemático real)

    // ESP32-C6FH8 co-processor (WiFi via esp-hosted), SDIO bus
    // TODO: verificar (esquemático real; asumidos iguales a la 3.5")
    static constexpr uint8_t C6_SDIO_CLK = GPIO_NUM_19;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_CMD = GPIO_NUM_18;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_D0  = GPIO_NUM_14;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_D1  = GPIO_NUM_15;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_D2  = GPIO_NUM_16;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_D3  = GPIO_NUM_17;  // TODO: verificar (esquemático real)
    static constexpr uint8_t C6_SDIO_RST = GPIO_NUM_54;  // TODO: verificar (esquemático real)
#endif

/**
 * @brief TFT Invert color
 *
 */
static constexpr bool TFT_INVERT = true;

void initHAL();

