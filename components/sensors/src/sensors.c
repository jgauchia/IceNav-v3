/**
 * @file sensors.c
 * @brief Unified sensor initialization and status for ESP-IDF
 */

#include "sensors.h"
#include "esp_log.h"

static const char *TAG = "sensors";

esp_err_t sensors_init_all(void)
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "Initializing all sensors...");
    ESP_LOGI(TAG, "================================");

    int sensors_ok = 0;
    int sensors_total = 4;

    // Initialize battery ADC
    if (battery_init() == ESP_OK) {
        battery_set_levels(4.2f, 3.3f);  // LiPo battery levels
        sensors_ok++;
    }

    // Initialize compass
    if (compass_init() == ESP_OK) {
        sensors_ok++;
    }

    // Initialize IMU
    if (imu_init() == ESP_OK) {
        sensors_ok++;
    }

    // Initialize barometer
    if (bme280_init() == ESP_OK) {
        sensors_ok++;
    }

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "Sensors initialized: %d/%d", sensors_ok, sensors_total);
    ESP_LOGI(TAG, "================================");

    return (sensors_ok > 0) ? ESP_OK : ESP_FAIL;
}

void sensors_print_status(void)
{
    ESP_LOGI(TAG, "========== SENSOR STATUS ==========");

    // Battery
    float voltage = battery_read_voltage();
    float percentage = battery_read_percentage();
    if (voltage > 0) {
        ESP_LOGI(TAG, "Battery: %.2fV (%.0f%%)", voltage, percentage);
    } else {
        ESP_LOGW(TAG, "Battery: NOT AVAILABLE");
    }

    // Compass
    if (compass_is_connected()) {
        compass_raw_t raw;
        if (compass_read_raw(&raw) == ESP_OK) {
            int heading = compass_get_heading(0, 0, 0);  // No calibration
            ESP_LOGI(TAG, "Compass: X=%d Y=%d Z=%d, Heading=%d deg", raw.x, raw.y, raw.z, heading);
        } else {
            ESP_LOGW(TAG, "Compass: READ ERROR");
        }
    } else {
        ESP_LOGW(TAG, "Compass: NOT CONNECTED");
    }

    // IMU
    if (imu_is_connected()) {
        imu_accel_t accel;
        imu_gyro_t gyro;
        if (imu_read_accel(&accel) == ESP_OK && imu_read_gyro(&gyro) == ESP_OK) {
            float temp = imu_read_temperature();
            ESP_LOGI(TAG, "IMU Accel: X=%.2fg Y=%.2fg Z=%.2fg", accel.x, accel.y, accel.z);
            ESP_LOGI(TAG, "IMU Gyro:  X=%.1f Y=%.1f Z=%.1f deg/s", gyro.x, gyro.y, gyro.z);
            ESP_LOGI(TAG, "IMU Temp:  %.1f C", temp);
        } else {
            ESP_LOGW(TAG, "IMU: READ ERROR");
        }
    } else {
        ESP_LOGW(TAG, "IMU: NOT CONNECTED");
    }

    // BME280
    if (bme280_is_connected()) {
        bme280_data_t data;
        if (bme280_read(&data) == ESP_OK) {
            ESP_LOGI(TAG, "BME280: Temp=%.1fC Press=%.1fhPa Hum=%.1f%% Alt=%.1fm",
                     data.temperature, data.pressure, data.humidity, data.altitude);
        } else {
            ESP_LOGW(TAG, "BME280: READ ERROR");
        }
    } else {
        ESP_LOGW(TAG, "BME280: NOT CONNECTED");
    }

    ESP_LOGI(TAG, "===================================");
}
