/**
 * @file i2c_espidf.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Native ESP-IDF I2C master wrapper - Replaces Arduino Wire.h
 * @version 0.2.4
 * @date 2025-12
 */

#include "i2c_espidf.hpp"
#include <esp_log.h>

static const char* TAG = "I2C";

// Global I2C instance
I2CNative i2c;

/**
 * @brief Constructs I2CNative with uninitialized state.
 */
I2CNative::I2CNative() : i2cPort(I2C_NUM_0), initialized(false)
{
}

/**
 * @brief Destructor - cleans up I2C resources.
 */
I2CNative::~I2CNative()
{
    end();
}

/**
 * @brief Initializes the I2C bus.
 */
bool I2CNative::begin(int sda, int scl, uint32_t freq)
{
    if (initialized)
        return true;

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)sda;
    conf.scl_io_num = (gpio_num_t)scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq;

    esp_err_t ret = i2c_param_config(i2cPort, &conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(ret));
        return false;
    }

    ret = i2c_driver_install(i2cPort, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "I2C bus initialized (SDA:%d, SCL:%d, %lu Hz)", sda, scl, freq);
    return true;
}

/**
 * @brief Deinitializes the I2C bus.
 */
void I2CNative::end()
{
    if (!initialized)
        return;

    i2c_driver_delete(i2cPort);
    initialized = false;
    ESP_LOGI(TAG, "I2C bus deinitialized");
}

/**
 * @brief Reads a single byte from a register.
 */
uint8_t I2CNative::read8(uint8_t addr, uint8_t reg)
{
    if (!initialized)
        return 0;

    uint8_t value = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Read8 failed for 0x%02X reg 0x%02X: %s", addr, reg, esp_err_to_name(ret));
        return 0;
    }

    return value;
}

/**
 * @brief Writes a single byte to a register.
 */
bool I2CNative::write8(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!initialized)
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Write8 failed for 0x%02X reg 0x%02X: %s", addr, reg, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief Reads multiple bytes from a register.
 */
size_t I2CNative::readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len)
{
    if (!initialized || !buffer || len == 0)
        return 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);

    if (len > 1)
        i2c_master_read(cmd, buffer, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buffer + len - 1, I2C_MASTER_NACK);

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ReadBytes failed for 0x%02X reg 0x%02X: %s", addr, reg, esp_err_to_name(ret));
        return 0;
    }

    return len;
}

/**
 * @brief Reads multiple bytes without register (direct read).
 */
size_t I2CNative::readBytesRaw(uint8_t addr, uint8_t* buffer, size_t len)
{
    if (!initialized || !buffer || len == 0)
        return 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);

    if (len > 1)
        i2c_master_read(cmd, buffer, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buffer + len - 1, I2C_MASTER_NACK);

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ReadBytesRaw failed for 0x%02X: %s", addr, esp_err_to_name(ret));
        return 0;
    }

    return len;
}

/**
 * @brief Writes multiple bytes to a register.
 */
bool I2CNative::writeBytes(uint8_t addr, uint8_t reg, const uint8_t* buffer, size_t len)
{
    if (!initialized || !buffer || len == 0)
        return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, buffer, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WriteBytes failed for 0x%02X reg 0x%02X: %s", addr, reg, esp_err_to_name(ret));
        return false;
    }

    return true;
}
