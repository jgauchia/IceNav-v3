/**
 * @file i2c_driver_base.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Base class for I2C peripheral drivers
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "i2c_espidf.hpp"
#include <cstdint>

/**
 * @class I2CDriverBase
 * @brief Provides common single-byte and multi-byte I2C helpers for peripheral drivers.
 *
 * @details Drivers inherit protected access to read8/write8/read16/read16LE,
 *          all delegating to the shared I2CNative bus instance. Each driver
 *          stores its own device address in i2cAddr.
 */
class I2CDriverBase
{
protected:
    uint8_t i2cAddr; /**< 7-bit I2C device address. */

    /**
     * @brief Reads a single byte from a register.
     *
     * @param reg Register address.
     * @return Register value.
     */
    uint8_t read8(uint8_t reg) { return i2c.read8(i2cAddr, reg); }

    /**
     * @brief Writes a single byte to a register.
     *
     * @param reg   Register address.
     * @param value Value to write.
     * @return true on success.
     */
    bool write8(uint8_t reg, uint8_t value) { return i2c.write8(i2cAddr, reg, value); }

    /**
     * @brief Reads a 16-bit value (MSB first).
     *
     * @param reg Starting register address.
     * @return 16-bit signed value.
     */
    int16_t read16(uint8_t reg)
    {
        uint8_t buf[2];
        i2c.readBytes(i2cAddr, reg, buf, 2);
        return static_cast<int16_t>((buf[0] << 8) | buf[1]);
    }

    /**
     * @brief Reads a 16-bit value (LSB first / little-endian).
     *
     * @param reg Starting register address.
     * @return 16-bit unsigned value (little-endian).
     */
    uint16_t read16LE(uint8_t reg)
    {
        uint8_t buf[2];
        i2c.readBytes(i2cAddr, reg, buf, 2);
        return static_cast<uint16_t>((buf[1] << 8) | buf[0]);
    }

    explicit I2CDriverBase(uint8_t addr) : i2cAddr(addr) {}
};
