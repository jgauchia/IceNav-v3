/**
 * @file sensors.h
 * @brief Unified sensor interface for IceNav ESP-IDF
 * @details Provides access to compass, IMU, barometer, and battery sensors
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Battery ADC
// ============================================================================

/**
 * @brief Initialize battery ADC
 * @return ESP_OK on success
 */
esp_err_t battery_init(void);

/**
 * @brief Set battery voltage thresholds
 * @param max_voltage Full charge voltage (e.g., 4.2V)
 * @param min_voltage Empty voltage (e.g., 3.3V)
 */
void battery_set_levels(float max_voltage, float min_voltage);

/**
 * @brief Read battery percentage
 * @return Battery percentage (0-100), or -1 on error
 */
float battery_read_percentage(void);

/**
 * @brief Read battery voltage
 * @return Battery voltage in volts
 */
float battery_read_voltage(void);

// ============================================================================
// QMC5883L Compass
// ============================================================================

#define QMC5883L_ADDR   0x0D    /**< QMC5883L I2C address */

/**
 * @brief Compass raw data
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} compass_raw_t;

/**
 * @brief Initialize QMC5883L compass
 * @return ESP_OK on success
 */
esp_err_t compass_init(void);

/**
 * @brief Check if compass is connected
 * @return true if connected
 */
bool compass_is_connected(void);

/**
 * @brief Read raw compass data
 * @param raw Pointer to store raw data
 * @return ESP_OK on success
 */
esp_err_t compass_read_raw(compass_raw_t *raw);

/**
 * @brief Get compass heading in degrees (0-360)
 * @param offset_x X-axis calibration offset
 * @param offset_y Y-axis calibration offset
 * @param declination Magnetic declination in radians
 * @return Heading in degrees
 */
int compass_get_heading(float offset_x, float offset_y, float declination);

// ============================================================================
// MPU6050 IMU
// ============================================================================

#define MPU6050_ADDR    0x68    /**< MPU6050 I2C address */

/**
 * @brief IMU accelerometer data (in g)
 */
typedef struct {
    float x;
    float y;
    float z;
} imu_accel_t;

/**
 * @brief IMU gyroscope data (in deg/s)
 */
typedef struct {
    float x;
    float y;
    float z;
} imu_gyro_t;

/**
 * @brief Initialize MPU6050 IMU
 * @return ESP_OK on success
 */
esp_err_t imu_init(void);

/**
 * @brief Check if IMU is connected
 * @return true if connected
 */
bool imu_is_connected(void);

/**
 * @brief Read accelerometer data
 * @param accel Pointer to store accelerometer data
 * @return ESP_OK on success
 */
esp_err_t imu_read_accel(imu_accel_t *accel);

/**
 * @brief Read gyroscope data
 * @param gyro Pointer to store gyroscope data
 * @return ESP_OK on success
 */
esp_err_t imu_read_gyro(imu_gyro_t *gyro);

/**
 * @brief Read temperature from IMU
 * @return Temperature in Celsius
 */
float imu_read_temperature(void);

// ============================================================================
// BME280 Barometer
// ============================================================================

#define BME280_ADDR     0x76    /**< BME280 I2C address */

/**
 * @brief BME280 environmental data
 */
typedef struct {
    float temperature;  /**< Temperature in Celsius */
    float pressure;     /**< Pressure in hPa (mbar) */
    float humidity;     /**< Relative humidity in % */
    float altitude;     /**< Estimated altitude in meters */
} bme280_data_t;

/**
 * @brief Initialize BME280 sensor
 * @return ESP_OK on success
 */
esp_err_t bme280_init(void);

/**
 * @brief Check if BME280 is connected
 * @return true if connected
 */
bool bme280_is_connected(void);

/**
 * @brief Read all BME280 data
 * @param data Pointer to store sensor data
 * @return ESP_OK on success
 */
esp_err_t bme280_read(bme280_data_t *data);

/**
 * @brief Read temperature only
 * @return Temperature in Celsius
 */
float bme280_read_temperature(void);

/**
 * @brief Read pressure only
 * @return Pressure in hPa
 */
float bme280_read_pressure(void);

/**
 * @brief Read humidity only
 * @return Relative humidity in %
 */
float bme280_read_humidity(void);

/**
 * @brief Set sea level pressure for altitude calculation
 * @param pressure Sea level pressure in hPa (default: 1013.25)
 */
void bme280_set_sea_level_pressure(float pressure);

// ============================================================================
// Unified Sensors
// ============================================================================

/**
 * @brief Initialize all sensors
 * @return ESP_OK if at least one sensor initialized successfully
 */
esp_err_t sensors_init_all(void);

/**
 * @brief Print sensor status to console
 */
void sensors_print_status(void);

#ifdef __cplusplus
}
#endif
