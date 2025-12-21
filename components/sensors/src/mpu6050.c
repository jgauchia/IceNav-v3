/**
 * @file mpu6050.c
 * @brief MPU6050 IMU driver for ESP-IDF (new I2C master driver)
 */

#include "sensors.h"
#include "board.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "imu";

// MPU6050 registers
#define MPU6050_REG_SMPLRT_DIV      0x19
#define MPU6050_REG_CONFIG          0x1A
#define MPU6050_REG_GYRO_CONFIG     0x1B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_FIFO_EN         0x23
#define MPU6050_REG_INT_PIN_CFG     0x37
#define MPU6050_REG_INT_ENABLE      0x38
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_ACCEL_XOUT_L    0x3C
#define MPU6050_REG_ACCEL_YOUT_H    0x3D
#define MPU6050_REG_ACCEL_YOUT_L    0x3E
#define MPU6050_REG_ACCEL_ZOUT_H    0x3F
#define MPU6050_REG_ACCEL_ZOUT_L    0x40
#define MPU6050_REG_TEMP_OUT_H      0x41
#define MPU6050_REG_TEMP_OUT_L      0x42
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_GYRO_XOUT_L     0x44
#define MPU6050_REG_GYRO_YOUT_H     0x45
#define MPU6050_REG_GYRO_YOUT_L     0x46
#define MPU6050_REG_GYRO_ZOUT_H     0x47
#define MPU6050_REG_GYRO_ZOUT_L     0x48
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_PWR_MGMT_2      0x6C
#define MPU6050_REG_WHO_AM_I        0x75

// Configuration values
#define MPU6050_WHO_AM_I_VAL        0x68
#define MPU6050_CLOCK_PLL_XGYRO     0x01
#define MPU6050_GYRO_FS_250         0x00
#define MPU6050_GYRO_FS_500         0x08
#define MPU6050_GYRO_FS_1000        0x10
#define MPU6050_GYRO_FS_2000        0x18
#define MPU6050_ACCEL_FS_2          0x00
#define MPU6050_ACCEL_FS_4          0x08
#define MPU6050_ACCEL_FS_8          0x10
#define MPU6050_ACCEL_FS_16         0x18

// Scale factors
#define ACCEL_SCALE_2G              16384.0f
#define GYRO_SCALE_250DPS           131.0f

static bool imu_connected = false;
static i2c_master_dev_handle_t mpu6050_handle = NULL;

// I2C helper functions using new master driver
static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(mpu6050_handle, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t mpu6050_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(mpu6050_handle, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(mpu6050_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t imu_init(void)
{
    ESP_LOGI(TAG, "Initializing MPU6050 IMU");

    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &mpu6050_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add MPU6050 device: %s", esp_err_to_name(ret));
        imu_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    // Check WHO_AM_I register
    uint8_t who_am_i;
    ret = mpu6050_read_reg(MPU6050_REG_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MPU6050 not responding at address 0x%02X", MPU6050_ADDR);
        i2c_master_bus_rm_device(mpu6050_handle);
        mpu6050_handle = NULL;
        imu_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "MPU6050 WHO_AM_I: 0x%02X", who_am_i);

    if (who_am_i != MPU6050_WHO_AM_I_VAL) {
        ESP_LOGW(TAG, "Unexpected WHO_AM_I value (expected 0x%02X)", MPU6050_WHO_AM_I_VAL);
        // Continue anyway, might be compatible device
    }

    // Wake up from sleep mode
    ret = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU6050");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // Set clock source to PLL with X axis gyroscope reference
    ret = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, MPU6050_CLOCK_PLL_XGYRO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set clock source");
        return ret;
    }

    // Configure gyroscope: ±250°/s
    ret = mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, MPU6050_GYRO_FS_250);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure gyroscope");
        return ret;
    }

    // Configure accelerometer: ±2g
    ret = mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_FS_2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure accelerometer");
        return ret;
    }

    // Set sample rate divider (sample rate = 1kHz / (1 + div))
    ret = mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x07);  // 125 Hz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate");
        return ret;
    }

    // Configure DLPF (Digital Low Pass Filter)
    ret = mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03);  // 44Hz bandwidth
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DLPF");
        return ret;
    }

    imu_connected = true;
    ESP_LOGI(TAG, "MPU6050 IMU initialized");
    return ESP_OK;
}

bool imu_is_connected(void)
{
    return imu_connected;
}

esp_err_t imu_read_accel(imu_accel_t *accel)
{
    if (!imu_connected || !accel || !mpu6050_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[6];
    esp_err_t ret = mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_y = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_z = (int16_t)((data[4] << 8) | data[5]);

    accel->x = raw_x / ACCEL_SCALE_2G;
    accel->y = raw_y / ACCEL_SCALE_2G;
    accel->z = raw_z / ACCEL_SCALE_2G;

    return ESP_OK;
}

esp_err_t imu_read_gyro(imu_gyro_t *gyro)
{
    if (!imu_connected || !gyro || !mpu6050_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[6];
    esp_err_t ret = mpu6050_read_regs(MPU6050_REG_GYRO_XOUT_H, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_y = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_z = (int16_t)((data[4] << 8) | data[5]);

    gyro->x = raw_x / GYRO_SCALE_250DPS;
    gyro->y = raw_y / GYRO_SCALE_250DPS;
    gyro->z = raw_z / GYRO_SCALE_250DPS;

    return ESP_OK;
}

float imu_read_temperature(void)
{
    if (!imu_connected || !mpu6050_handle) {
        return -999.0f;
    }

    uint8_t data[2];
    esp_err_t ret = mpu6050_read_regs(MPU6050_REG_TEMP_OUT_H, data, 2);
    if (ret != ESP_OK) {
        return -999.0f;
    }

    int16_t raw_temp = (int16_t)((data[0] << 8) | data[1]);

    // Temperature in degrees C = (TEMP_OUT / 340) + 36.53
    float temperature = (raw_temp / 340.0f) + 36.53f;

    return temperature;
}
