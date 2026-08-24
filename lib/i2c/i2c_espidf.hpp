/**
 * @file i2c_espidf.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// I2C Configuration
// Speed: 100000=100kHz (standard), 400000=400kHz (fast)

class I2CNative
{
public:
    I2CNative();
    ~I2CNative();

    bool begin(int sda, int scl, uint32_t freq = 400000);
    void end();
    uint8_t read8(uint8_t addr, uint8_t reg);
    bool write8(uint8_t addr, uint8_t reg, uint8_t value);
    size_t readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len);
    size_t readBytesRaw(uint8_t addr, uint8_t* buffer, size_t len);
    bool writeBytes(uint8_t addr, uint8_t reg, const uint8_t* buffer, size_t len);
    bool isInitialized() const { return initialized; }

    bool lock(TickType_t timeout = pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS))
    {
        if (!initialized || i2cMutex == nullptr)
            return false;
        return xSemaphoreTake(i2cMutex, timeout) == pdTRUE;
    }

    void unlock()
    {
        if (initialized && i2cMutex != nullptr)
            xSemaphoreGive(i2cMutex);
    }

private:
    i2c_master_bus_handle_t busHandle;
    uint32_t busFreq;
    bool initialized;
    SemaphoreHandle_t i2cMutex;
    static constexpr int I2C_MUTEX_TIMEOUT_MS = 15;
    static constexpr int I2C_BUS_TIMEOUT_MS   = 20;

    // The new i2c_master driver requires one device handle per address.
    // Handles are created lazily and cached; the sensor set is small.
    static constexpr int MAX_DEVICES = 8;
    struct DeviceEntry
    {
        uint8_t addr;
        i2c_master_dev_handle_t handle;
    };
    DeviceEntry devices[MAX_DEVICES];
    int deviceCount;

    i2c_master_dev_handle_t deviceFor(uint8_t addr);
};

extern I2CNative i2c;
