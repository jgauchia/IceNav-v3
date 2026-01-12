/**
 * @file i2c_espidf.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "driver/i2c.h"
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

private:
    i2c_port_t i2cPort;
    bool initialized;
    SemaphoreHandle_t i2cMutex;
    static constexpr int I2C_TIMEOUT_MS = 200;
};

extern I2CNative i2c;
