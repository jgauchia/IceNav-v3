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
 * @details Initializes the I2C port identifier to I2C_NUM_0 by default and resets the mutex.
 */
I2CNative::I2CNative() : i2cPort(I2C_NUM_0), initialized(false), i2cMutex(nullptr)
{
}

/**
 * @brief Destructor - cleans up I2C resources.
 * @details Ensures the I2C driver is uninstalled and resources are freed.
 */
I2CNative::~I2CNative()
{
    end();
}

/**
 * @brief Initializes the I2C bus with specific pins and frequency.
 * @param sda  GPIO pin number for SDA line.
 * @param scl  GPIO pin number for SCL line.
 * @param freq I2C clock frequency in Hz.
 * @return true if initialization was successful, false on error.
 */
bool I2CNative::begin(int sda, int scl, uint32_t freq)
{
    if (initialized)
        return true;

    if (i2cMutex == nullptr)
    {
        i2cMutex = xSemaphoreCreateMutex();
    }

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
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return false;
    }

    i2c_set_timeout(i2cPort, 0xFFFFF); 
    i2c_filter_enable(i2cPort, 7);
    if (i2c_set_period(i2cPort, conf.master.clk_speed / 2, conf.master.clk_speed / 4) != ESP_OK)
        ESP_LOGW(TAG, "Failed to set I2C timing period");

    initialized = true;
    ESP_LOGI(TAG, "I2C bus initialized (SDA:%d, SCL:%d, %lu Hz)", sda, scl, freq);
    return true;
}

/**
 * @brief Deinitializes the I2C bus.
 * @details Deletes the I2C driver and marks the instance as uninitialized.
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
 * @param addr I2C device address.
 * @param reg  Register address to read from.
 * @return Value read from the register, or 0 on error.
 */
uint8_t I2CNative::read8(uint8_t addr, uint8_t reg)
{
    if (!initialized || i2cMutex == nullptr)
        return 0;

    uint8_t value = 0;
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE)
    {
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
            value = 0;
        }
        xSemaphoreGive(i2cMutex);
    }

    return value;
}

/**
 * @brief Writes a single byte to a register.
 * @param addr  I2C device address.
 * @param reg   Register address to write to.
 * @param value Value to write to the register.
 * @return true if write successful, false on error.
 */
bool I2CNative::write8(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!initialized || i2cMutex == nullptr)
        return false;

    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();

            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(cmd, reg, true);
            i2c_master_write_byte(cmd, value, true);
            i2c_master_stop(cmd);

            ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            i2c_cmd_link_delete(cmd);
            xSemaphoreGive(i2cMutex);

            if (ret == ESP_OK)
            {
                return true;
            }
            ESP_LOGW(TAG, "Write8 failed for 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(5 * (i + 1)));
        }
        ESP_LOGW(TAG, "Write8 failed to take mutex for 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
        vTaskDelay(pdMS_TO_TICKS(10 * (i + 1)));
    }

    ESP_LOGE(TAG, "Write8 failed for 0x%02X reg 0x%02X after %d retries. Last error: %s", addr, reg, retries, esp_err_to_name(ret));
    return false; // Todos los reintentos fallaron
}

/**
 * @brief Reads multiple bytes from a register.
 * @param addr   I2C device address.
 * @param reg    Register address to start reading from.
 * @param buffer Pointer to the buffer to store data.
 * @param len    Number of bytes to read.
 * @return Number of bytes successfully read.
 */
size_t I2CNative::readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len)
{
    if (!initialized || i2cMutex == nullptr || !buffer || len == 0)
        return 0;

    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE)
        {
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

            ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            i2c_cmd_link_delete(cmd);
            xSemaphoreGive(i2cMutex);

            if (ret == ESP_OK)
            {
                return len;
            }
            ESP_LOGW(TAG, "ReadBytes failed for 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(5 * (i + 1)));
        }
        ESP_LOGW(TAG, "ReadBytes failed to take mutex for 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
        vTaskDelay(pdMS_TO_TICKS(10 * (i + 1)));
    }

    ESP_LOGE(TAG, "ReadBytes failed for 0x%02X reg 0x%02X after %d retries. Last error: %s", addr, reg, retries, esp_err_to_name(ret));
    return 0; // Todos los reintentos fallaron
}

/**
 * @brief Reads multiple bytes without register (direct read).
 * @param addr   I2C device address.
 * @param buffer Pointer to the buffer to store data.
 * @param len    Number of bytes to read.
 * @return Number of bytes successfully read.
 */
size_t I2CNative::readBytesRaw(uint8_t addr, uint8_t* buffer, size_t len)
{
    if (!initialized || i2cMutex == nullptr || !buffer || len == 0)
        return 0;

    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();

            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);

            if (len > 1)
                i2c_master_read(cmd, buffer, len - 1, I2C_MASTER_ACK);
            i2c_master_read_byte(cmd, buffer + len - 1, I2C_MASTER_NACK);

            i2c_master_stop(cmd);

            ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            i2c_cmd_link_delete(cmd);
            xSemaphoreGive(i2cMutex);

            if (ret == ESP_OK)
            {
                return len;
            }
            ESP_LOGW(TAG, "ReadBytesRaw failed for 0x%02X (attempt %d/%d): %s", addr, i + 1, retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(5 * (i + 1)));
        }
        ESP_LOGW(TAG, "ReadBytesRaw failed to take mutex for 0x%02X (attempt %d/%d)", addr, i + 1, retries);
        vTaskDelay(pdMS_TO_TICKS(10 * (i + 1)));
    }

    ESP_LOGE(TAG, "ReadBytesRaw failed for 0x%02X after %d retries. Last error: %s", addr, retries, esp_err_to_name(ret));
    return 0; // Todos los reintentos fallaron
}

/**
 * @brief Writes multiple bytes to a register.
 * @param addr   I2C device address.
 * @param reg    Register address to start writing to.
 * @param buffer Pointer to the data buffer to write.
 * @param len    Number of bytes to write.
 * @return true if write successful, false on error.
 */
bool I2CNative::writeBytes(uint8_t addr, uint8_t reg, const uint8_t* buffer, size_t len)
{
    if (!initialized || i2cMutex == nullptr || !buffer || len == 0)
        return false;

    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();

            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(cmd, reg, true);
            i2c_master_write(cmd, buffer, len, true);
            i2c_master_stop(cmd);

            ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
            i2c_cmd_link_delete(cmd);
            xSemaphoreGive(i2cMutex);

            if (ret == ESP_OK)
            {
                return true;
            }
            ESP_LOGW(TAG, "WriteBytes failed for 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(5 * (i + 1)));
        }
        ESP_LOGW(TAG, "WriteBytes failed to take mutex for 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
        vTaskDelay(pdMS_TO_TICKS(10 * (i + 1)));
    }

    ESP_LOGE(TAG, "WriteBytes failed for 0x%02X reg 0x%02X after %d retries. Last error: %s", addr, reg, retries, esp_err_to_name(ret));
    return false; // Todos los reintentos fallaron
}
