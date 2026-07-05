/**
 * @file i2c_espidf.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Native ESP-IDF I2C master wrapper - Replaces Arduino Wire.h
 * @version 0.3.0
 * @date 2026-06
 */

#include "i2c_espidf.hpp"
#include <esp_log.h>

static const char* TAG = "I2C";

// Global I2C instance
I2CNative i2c;

/**
 * @brief Constructs I2CNative with uninitialized state.
 *
 * @details Resets the bus handle, device cache and mutex.
 */
I2CNative::I2CNative() : busHandle(nullptr), busFreq(0), initialized(false), i2cMutex(nullptr), deviceCount(0)
{
}

/**
 * @brief Destructor - cleans up I2C resources.
 *
 * @details Ensures the I2C bus and device handles are removed and resources are freed.
 */
I2CNative::~I2CNative()
{
    end();
}

/**
 * @brief Initializes the I2C bus with specific pins and frequency.
 *
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
        i2cMutex = xSemaphoreCreateMutex();

    i2c_master_bus_config_t busCfg = {};
    busCfg.i2c_port = I2C_NUM_0;
    busCfg.sda_io_num = (gpio_num_t)sda;
    busCfg.scl_io_num = (gpio_num_t)scl;
    busCfg.clk_source = I2C_CLK_SRC_DEFAULT;
    busCfg.glitch_ignore_cnt = 7;
    busCfg.flags.enable_internal_pullup = true;

    esp_err_t ret = i2c_new_master_bus(&busCfg, &busHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return false;
    }

    busFreq = freq;
    deviceCount = 0;
    initialized = true;
    ESP_LOGI(TAG, "I2C bus initialized (SDA:%d, SCL:%d, %lu Hz)", sda, scl, freq);
    return true;
}

/**
 * @brief Deinitializes the I2C bus.
 *
 * @details Removes every cached device handle and deletes the master bus.
 */
void I2CNative::end()
{
    if (!initialized)
        return;

    for (int i = 0; i < deviceCount; ++i)
        i2c_master_bus_rm_device(devices[i].handle);
    deviceCount = 0;

    i2c_del_master_bus(busHandle);
    busHandle = nullptr;
    initialized = false;
    ESP_LOGI(TAG, "I2C bus deinitialized");
}

/**
 * @brief Returns the cached device handle for an address, creating it on first use.
 *
 * @param addr 7-bit I2C device address.
 * @return Device handle, or nullptr if the cache is full or creation failed.
 */
i2c_master_dev_handle_t I2CNative::deviceFor(uint8_t addr)
{
    for (int i = 0; i < deviceCount; ++i)
        if (devices[i].addr == addr)
            return devices[i].handle;

    if (deviceCount >= MAX_DEVICES)
    {
        ESP_LOGE(TAG, "Device cache full, cannot add 0x%02X", addr);
        return nullptr;
    }

    i2c_device_config_t devCfg = {};
    devCfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devCfg.device_address = addr;
    devCfg.scl_speed_hz = busFreq;

    i2c_master_dev_handle_t handle = nullptr;
    esp_err_t ret = i2c_master_bus_add_device(busHandle, &devCfg, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add device 0x%02X: %s", addr, esp_err_to_name(ret));
        return nullptr;
    }

    devices[deviceCount].addr = addr;
    devices[deviceCount].handle = handle;
    deviceCount++;
    return handle;
}

/**
 * @brief Reads a single byte from a register.
 *
 * @param addr I2C device address.
 * @param reg  Register address to read from.
 * @return Value read from the register, or 0 on error.
 */
uint8_t I2CNative::read8(uint8_t addr, uint8_t reg)
{
    if (!initialized || i2cMutex == nullptr)
        return 0;

    uint8_t value = 0;
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        i2c_master_dev_handle_t dev = deviceFor(addr);
        if (dev != nullptr)
        {
            esp_err_t ret = i2c_master_transmit_receive(dev, &reg, 1, &value, 1, I2C_BUS_TIMEOUT_MS);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Read8 failed for 0x%02X reg 0x%02X: %s", addr, reg, esp_err_to_name(ret));
                value = 0;
            }
        }
        xSemaphoreGive(i2cMutex);
    }

    return value;
}

/**
 * @brief Writes a single byte to a register.
 *
 * @param addr  I2C device address.
 * @param reg   Register address to write to.
 * @param value Value to write to the register.
 * @return true if write successful, false on error.
 */
bool I2CNative::write8(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!initialized || i2cMutex == nullptr)
        return false;

    const uint8_t payload[2] = { reg, value };
    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGW(TAG, "Write8 mutex timeout 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        i2c_master_dev_handle_t dev = deviceFor(addr);
        if (dev != nullptr)
            ret = i2c_master_transmit(dev, payload, sizeof(payload), I2C_BUS_TIMEOUT_MS);
        xSemaphoreGive(i2cMutex);

        if (ret == ESP_OK)
            return true;

        ESP_LOGW(TAG, "Write8 failed 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGE(TAG, "Write8 failed 0x%02X reg 0x%02X after %d retries: %s", addr, reg, retries, esp_err_to_name(ret));
    return false;
}

/**
 * @brief Reads multiple bytes from a register.
 *
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
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGW(TAG, "ReadBytes mutex timeout 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        i2c_master_dev_handle_t dev = deviceFor(addr);
        if (dev != nullptr)
            ret = i2c_master_transmit_receive(dev, &reg, 1, buffer, len, I2C_BUS_TIMEOUT_MS);
        xSemaphoreGive(i2cMutex);

        if (ret == ESP_OK)
            return len;

        ESP_LOGW(TAG, "ReadBytes failed 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGE(TAG, "ReadBytes failed 0x%02X reg 0x%02X after %d retries: %s", addr, reg, retries, esp_err_to_name(ret));
    return 0;
}

/**
 * @brief Reads multiple bytes without register (direct read).
 *
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
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGW(TAG, "ReadBytesRaw mutex timeout 0x%02X (attempt %d/%d)", addr, i + 1, retries);
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        i2c_master_dev_handle_t dev = deviceFor(addr);
        if (dev != nullptr)
            ret = i2c_master_receive(dev, buffer, len, I2C_BUS_TIMEOUT_MS);
        xSemaphoreGive(i2cMutex);

        if (ret == ESP_OK)
            return len;

        ESP_LOGW(TAG, "ReadBytesRaw failed 0x%02X (attempt %d/%d): %s", addr, i + 1, retries, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGE(TAG, "ReadBytesRaw failed 0x%02X after %d retries: %s", addr, retries, esp_err_to_name(ret));
    return 0;
}

/**
 * @brief Writes multiple bytes to a register.
 *
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

    static constexpr size_t MAX_WRITE = 32;
    if (len > MAX_WRITE)
    {
        ESP_LOGE(TAG, "WriteBytes 0x%02X reg 0x%02X too long (%u > %u)", addr, reg, (unsigned)len, (unsigned)MAX_WRITE);
        return false;
    }

    uint8_t payload[1 + MAX_WRITE];
    payload[0] = reg;
    for (size_t i = 0; i < len; ++i)
        payload[1 + i] = buffer[i];

    esp_err_t ret = ESP_FAIL;
    int retries = 3;

    for (int i = 0; i < retries; ++i)
    {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGW(TAG, "WriteBytes mutex timeout 0x%02X reg 0x%02X (attempt %d/%d)", addr, reg, i + 1, retries);
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        i2c_master_dev_handle_t dev = deviceFor(addr);
        if (dev != nullptr)
            ret = i2c_master_transmit(dev, payload, len + 1, I2C_BUS_TIMEOUT_MS);
        xSemaphoreGive(i2cMutex);

        if (ret == ESP_OK)
            return true;

        ESP_LOGW(TAG, "WriteBytes failed 0x%02X reg 0x%02X (attempt %d/%d): %s", addr, reg, i + 1, retries, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGE(TAG, "WriteBytes failed 0x%02X reg 0x%02X after %d retries: %s", addr, reg, retries, esp_err_to_name(ret));
    return false;
}
