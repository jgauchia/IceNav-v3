/**
 * @file qmc5883l.c
 * @brief QMC5883L Compass driver for ESP-IDF (new I2C master driver)
 */

#include "sensors.h"
#include "board.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <math.h>

static const char *TAG = "compass";

// QMC5883L registers
#define QMC5883L_REG_DATA_X_LSB     0x00
#define QMC5883L_REG_DATA_X_MSB     0x01
#define QMC5883L_REG_DATA_Y_LSB     0x02
#define QMC5883L_REG_DATA_Y_MSB     0x03
#define QMC5883L_REG_DATA_Z_LSB     0x04
#define QMC5883L_REG_DATA_Z_MSB     0x05
#define QMC5883L_REG_STATUS         0x06
#define QMC5883L_REG_TEMP_LSB       0x07
#define QMC5883L_REG_TEMP_MSB       0x08
#define QMC5883L_REG_CONTROL1       0x09
#define QMC5883L_REG_CONTROL2       0x0A
#define QMC5883L_REG_SET_RESET      0x0B
#define QMC5883L_REG_CHIP_ID        0x0D

// Control register 1 values
#define QMC5883L_MODE_STANDBY       0x00
#define QMC5883L_MODE_CONTINUOUS    0x01
#define QMC5883L_ODR_10HZ           0x00
#define QMC5883L_ODR_50HZ           0x04
#define QMC5883L_ODR_100HZ          0x08
#define QMC5883L_ODR_200HZ          0x0C
#define QMC5883L_RNG_2G             0x00
#define QMC5883L_RNG_8G             0x10
#define QMC5883L_OSR_512            0x00
#define QMC5883L_OSR_256            0x40
#define QMC5883L_OSR_128            0x80
#define QMC5883L_OSR_64             0xC0

// Status register bits
#define QMC5883L_STATUS_DRDY        0x01
#define QMC5883L_STATUS_OVL         0x02
#define QMC5883L_STATUS_DOR         0x04

static bool compass_connected = false;
static i2c_master_dev_handle_t qmc5883l_handle = NULL;

// I2C helper functions using new master driver
static esp_err_t qmc5883l_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(qmc5883l_handle, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t qmc5883l_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(qmc5883l_handle, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

static esp_err_t qmc5883l_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(qmc5883l_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t compass_init(void)
{
    ESP_LOGI(TAG, "Initializing QMC5883L compass");

    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMC5883L_ADDR,
        .scl_speed_hz = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &qmc5883l_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add QMC5883L device: %s", esp_err_to_name(ret));
        compass_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    // Check chip ID
    uint8_t chip_id;
    ret = qmc5883l_read_reg(QMC5883L_REG_CHIP_ID, &chip_id);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "QMC5883L not responding at address 0x%02X", QMC5883L_ADDR);
        i2c_master_bus_rm_device(qmc5883l_handle);
        qmc5883l_handle = NULL;
        compass_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "QMC5883L chip ID: 0x%02X", chip_id);

    // Soft reset
    ret = qmc5883l_write_reg(QMC5883L_REG_CONTROL2, 0x80);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset QMC5883L");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set/Reset period
    ret = qmc5883l_write_reg(QMC5883L_REG_SET_RESET, 0x01);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set period");
        return ret;
    }

    // Configure: Continuous mode, 10Hz ODR, 2G range, 512 oversampling
    uint8_t config = QMC5883L_MODE_CONTINUOUS | QMC5883L_ODR_10HZ | QMC5883L_RNG_2G | QMC5883L_OSR_512;
    ret = qmc5883l_write_reg(QMC5883L_REG_CONTROL1, config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure QMC5883L");
        return ret;
    }

    compass_connected = true;
    ESP_LOGI(TAG, "QMC5883L compass initialized");
    return ESP_OK;
}

bool compass_is_connected(void)
{
    return compass_connected;
}

esp_err_t compass_read_raw(compass_raw_t *raw)
{
    if (!compass_connected || !raw || !qmc5883l_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    // Wait for data ready
    uint8_t status;
    int timeout = 100;
    do {
        esp_err_t ret = qmc5883l_read_reg(QMC5883L_REG_STATUS, &status);
        if (ret != ESP_OK) {
            return ret;
        }
        if (--timeout == 0) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1);
    } while (!(status & QMC5883L_STATUS_DRDY));

    // Read all 6 data bytes
    uint8_t data[6];
    esp_err_t ret = qmc5883l_read_regs(QMC5883L_REG_DATA_X_LSB, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    raw->x = (int16_t)(data[1] << 8 | data[0]);
    raw->y = (int16_t)(data[3] << 8 | data[2]);
    raw->z = (int16_t)(data[5] << 8 | data[4]);

    // Invert Y axis for ICENAV_BOARD orientation
#ifdef ICENAV_BOARD
    raw->y = -raw->y;
#endif

    return ESP_OK;
}

int compass_get_heading(float offset_x, float offset_y, float declination)
{
    compass_raw_t raw;
    if (compass_read_raw(&raw) != ESP_OK) {
        return -1;
    }

    // Apply calibration offsets
    float hx = (float)raw.x - offset_x;
    float hy = (float)raw.y - offset_y;

    // Calculate heading
    float heading = atan2f(hy, hx);
    heading += declination;

    // Wrap to [-PI, PI]
    while (heading < -M_PI) heading += 2 * M_PI;
    while (heading > M_PI) heading -= 2 * M_PI;

    // Convert to degrees (0-360)
    int heading_deg = (int)(heading * (180.0f / M_PI));
    if (heading_deg < 0) {
        heading_deg += 360;
    }

    return heading_deg;
}
