/**
 * @file bme280.c
 * @brief BME280 Barometer/Humidity/Temperature sensor driver for ESP-IDF (new I2C master driver)
 */

#include "sensors.h"
#include "board.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <math.h>

static const char *TAG = "bme280";

// BME280 registers
#define BME280_REG_CALIB00          0x88
#define BME280_REG_CALIB26          0xE1
#define BME280_REG_CHIP_ID          0xD0
#define BME280_REG_RESET            0xE0
#define BME280_REG_CTRL_HUM         0xF2
#define BME280_REG_STATUS           0xF3
#define BME280_REG_CTRL_MEAS        0xF4
#define BME280_REG_CONFIG           0xF5
#define BME280_REG_PRESS_MSB        0xF7
#define BME280_REG_PRESS_LSB        0xF8
#define BME280_REG_PRESS_XLSB       0xF9
#define BME280_REG_TEMP_MSB         0xFA
#define BME280_REG_TEMP_LSB         0xFB
#define BME280_REG_TEMP_XLSB        0xFC
#define BME280_REG_HUM_MSB          0xFD
#define BME280_REG_HUM_LSB          0xFE

// Configuration values
#define BME280_CHIP_ID              0x60
#define BME280_SOFT_RESET           0xB6

// Oversampling settings
#define BME280_OS_SKIP              0x00
#define BME280_OS_1X                0x01
#define BME280_OS_2X                0x02
#define BME280_OS_4X                0x03
#define BME280_OS_8X                0x04
#define BME280_OS_16X               0x05

// Mode settings
#define BME280_MODE_SLEEP           0x00
#define BME280_MODE_FORCED          0x01
#define BME280_MODE_NORMAL          0x03

// Calibration data structure
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bool bme280_connected = false;
static bme280_calib_t calib_data;
static int32_t t_fine;  // Fine temperature for compensation
static float sea_level_pressure = 1013.25f;
static i2c_master_dev_handle_t bme280_handle = NULL;

// I2C helper functions using new master driver
static esp_err_t bme280_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(bme280_handle, data, 2, pdMS_TO_TICKS(100));
}

static esp_err_t bme280_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(bme280_handle, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(bme280_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t bme280_read_calibration(void)
{
    uint8_t data[26];

    // Read calibration data 0x88-0xA1 (26 bytes)
    esp_err_t ret = bme280_read_regs(BME280_REG_CALIB00, data, 26);
    if (ret != ESP_OK) {
        return ret;
    }

    calib_data.dig_T1 = (uint16_t)(data[1] << 8 | data[0]);
    calib_data.dig_T2 = (int16_t)(data[3] << 8 | data[2]);
    calib_data.dig_T3 = (int16_t)(data[5] << 8 | data[4]);
    calib_data.dig_P1 = (uint16_t)(data[7] << 8 | data[6]);
    calib_data.dig_P2 = (int16_t)(data[9] << 8 | data[8]);
    calib_data.dig_P3 = (int16_t)(data[11] << 8 | data[10]);
    calib_data.dig_P4 = (int16_t)(data[13] << 8 | data[12]);
    calib_data.dig_P5 = (int16_t)(data[15] << 8 | data[14]);
    calib_data.dig_P6 = (int16_t)(data[17] << 8 | data[16]);
    calib_data.dig_P7 = (int16_t)(data[19] << 8 | data[18]);
    calib_data.dig_P8 = (int16_t)(data[21] << 8 | data[20]);
    calib_data.dig_P9 = (int16_t)(data[23] << 8 | data[22]);
    calib_data.dig_H1 = data[25];

    // Read calibration data 0xE1-0xE7 (7 bytes)
    uint8_t hum_data[7];
    ret = bme280_read_regs(BME280_REG_CALIB26, hum_data, 7);
    if (ret != ESP_OK) {
        return ret;
    }

    calib_data.dig_H2 = (int16_t)(hum_data[1] << 8 | hum_data[0]);
    calib_data.dig_H3 = hum_data[2];
    calib_data.dig_H4 = (int16_t)((hum_data[3] << 4) | (hum_data[4] & 0x0F));
    calib_data.dig_H5 = (int16_t)((hum_data[5] << 4) | (hum_data[4] >> 4));
    calib_data.dig_H6 = (int8_t)hum_data[6];

    return ESP_OK;
}

// Compensation formulas from BME280 datasheet
static float bme280_compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) *
            ((int32_t)calib_data.dig_T3)) >> 14;

    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

static float bme280_compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 * (int64_t)calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);

    return (float)p / 25600.0f;  // Return in hPa
}

static float bme280_compensate_humidity(int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) -
                   (((int32_t)calib_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)calib_data.dig_H2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                              ((int32_t)calib_data.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (float)(v_x1_u32r >> 12) / 1024.0f;
}

esp_err_t bme280_init(void)
{
    ESP_LOGI(TAG, "Initializing BME280 sensor");

    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_ADDR,
        .scl_speed_hz = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &bme280_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add BME280 device: %s", esp_err_to_name(ret));
        bme280_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    // Check chip ID
    uint8_t chip_id;
    ret = bme280_read_reg(BME280_REG_CHIP_ID, &chip_id);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BME280 not responding at address 0x%02X", BME280_ADDR);
        i2c_master_bus_rm_device(bme280_handle);
        bme280_handle = NULL;
        bme280_connected = false;
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "BME280 chip ID: 0x%02X", chip_id);

    if (chip_id != BME280_CHIP_ID) {
        ESP_LOGW(TAG, "Unexpected chip ID (expected 0x%02X)", BME280_CHIP_ID);
        // Could be BMP280 (0x58) - continue anyway
    }

    // Soft reset
    ret = bme280_write_reg(BME280_REG_RESET, BME280_SOFT_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset BME280");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Wait for reset complete
    uint8_t status;
    int timeout = 100;
    do {
        ret = bme280_read_reg(BME280_REG_STATUS, &status);
        if (ret != ESP_OK) {
            return ret;
        }
        if (--timeout == 0) {
            ESP_LOGE(TAG, "BME280 reset timeout");
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    } while (status & 0x01);  // Wait until im_update bit is 0

    // Read calibration data
    ret = bme280_read_calibration();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read calibration data");
        return ret;
    }

    // Configure humidity oversampling (must be set before ctrl_meas)
    ret = bme280_write_reg(BME280_REG_CTRL_HUM, BME280_OS_1X);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure humidity");
        return ret;
    }

    // Configure: standby 1000ms, filter off
    ret = bme280_write_reg(BME280_REG_CONFIG, 0xA0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure standby/filter");
        return ret;
    }

    // Configure: temp x1, pressure x1, normal mode
    ret = bme280_write_reg(BME280_REG_CTRL_MEAS, (BME280_OS_1X << 5) | (BME280_OS_1X << 2) | BME280_MODE_NORMAL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure mode");
        return ret;
    }

    bme280_connected = true;
    ESP_LOGI(TAG, "BME280 sensor initialized");
    return ESP_OK;
}

bool bme280_is_connected(void)
{
    return bme280_connected;
}

esp_err_t bme280_read(bme280_data_t *data)
{
    if (!bme280_connected || !data || !bme280_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    // Read all measurement registers (8 bytes: press[3] + temp[3] + hum[2])
    uint8_t raw[8];
    esp_err_t ret = bme280_read_regs(BME280_REG_PRESS_MSB, raw, 8);
    if (ret != ESP_OK) {
        return ret;
    }

    // Extract raw ADC values
    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

    // Compensate readings (temperature must be first to set t_fine)
    data->temperature = bme280_compensate_temperature(adc_T);
    data->pressure = bme280_compensate_pressure(adc_P);
    data->humidity = bme280_compensate_humidity(adc_H);

    // Calculate altitude using barometric formula
    // h = 44330 * (1 - (P/P0)^(1/5.255))
    data->altitude = 44330.0f * (1.0f - powf(data->pressure / sea_level_pressure, 0.1903f));

    return ESP_OK;
}

float bme280_read_temperature(void)
{
    bme280_data_t data;
    if (bme280_read(&data) != ESP_OK) {
        return -999.0f;
    }
    return data.temperature;
}

float bme280_read_pressure(void)
{
    bme280_data_t data;
    if (bme280_read(&data) != ESP_OK) {
        return -1.0f;
    }
    return data.pressure;
}

float bme280_read_humidity(void)
{
    bme280_data_t data;
    if (bme280_read(&data) != ESP_OK) {
        return -1.0f;
    }
    return data.humidity;
}

void bme280_set_sea_level_pressure(float pressure)
{
    sea_level_pressure = pressure;
}
