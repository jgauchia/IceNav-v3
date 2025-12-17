/**
 * @file board.h
 * @brief Board HAL - Pin definitions and bus initialization
 */

#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ICENAV_BOARD Pin Definitions
// =============================================================================
#ifdef ICENAV_BOARD

// I2C Pins
#define BOARD_I2C_SDA         GPIO_NUM_38
#define BOARD_I2C_SCL         GPIO_NUM_39
#define BOARD_I2C_FREQ_HZ     400000

// GPS UART Pins
#define BOARD_GPS_UART_NUM    UART_NUM_1
#define BOARD_GPS_TX          GPIO_NUM_43
#define BOARD_GPS_RX          GPIO_NUM_44
#define BOARD_GPS_BAUD        9600

// SD Card SPI Pins
#define BOARD_SD_CS           GPIO_NUM_1
#define BOARD_SD_MISO         GPIO_NUM_41
#define BOARD_SD_MOSI         GPIO_NUM_2
#define BOARD_SD_CLK          GPIO_NUM_42

// Boot Pin
#define BOARD_BOOT_PIN        GPIO_NUM_0

#endif // ICENAV_BOARD

// =============================================================================
// Bus Handles (extern)
// =============================================================================
extern i2c_master_bus_handle_t i2c_bus_handle;

// =============================================================================
// Board Functions
// =============================================================================

/**
 * @brief Initialize all hardware buses
 * @return ESP_OK on success
 */
esp_err_t board_init(void);

/**
 * @brief Initialize I2C bus
 * @return ESP_OK on success
 */
esp_err_t board_i2c_init(void);

/**
 * @brief Initialize SPI bus for SD card
 * @return ESP_OK on success
 */
esp_err_t board_spi_init(void);

/**
 * @brief Initialize UART for GPS
 * @return ESP_OK on success
 */
esp_err_t board_uart_init(void);

#ifdef __cplusplus
}
#endif
