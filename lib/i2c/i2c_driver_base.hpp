/**
 * @file i2c_driver_base.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Base class for I2C peripheral drivers
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include "i2c_espidf.hpp"
#include <cstdint>
#include <cstddef>

// Pulls in the board's TOUCH_FT5x06/TOUCH_GT911/TOUCH_XPT2046 -> TOUCH_CAPACITIVE/
// TOUCH_RESISTIVE macros (lgfxCommon.hpp) regardless of which .cpp includes this
// header first — TOUCH_CAPACITIVE is not a global build flag, so it must be
// resolved here rather than relying on some other header having been included
// earlier in the same translation unit.
#include "panelSelect.hpp"

#ifdef TOUCH_CAPACITIVE
    #include <LovyanGFX.hpp>
#endif

/**
 * @class I2CDriverBase
 * @brief Provides common single-byte and multi-byte I2C helpers for peripheral drivers.
 *
 * @details Drivers inherit protected access to read8/write8/read16/read16LE/
 *          readBytes. On boards where the touch panel is I2C (TOUCH_CAPACITIVE),
 *          calling beginShared() makes every helper dispatch to lgfx::i2c, reusing
 *          the bus already owned by LovyanGFX. Otherwise (or before beginShared()
 *          is called) helpers delegate to the standalone I2CNative bus instance.
 *          Each driver stores its own device address in i2cAddr.
 */
class I2CDriverBase
{
protected:
    uint8_t i2cAddr; /**< 7-bit I2C device address. */

    #ifdef TOUCH_CAPACITIVE
        int i2cPort = -1; /**< lgfx I2C port once beginShared() is called, -1 otherwise. */

        /**
         * @brief Shares the I2C bus already owned by LovyanGFX (touch panel bus).
         *
         * @param port lgfx I2C port already initialized by LovyanGFX.
         */
        void beginShared(int port) { i2cPort = port; }
    #endif

    /**
     * @brief Reads a single byte from a register.
     *
     * @param reg Register address.
     * @return Register value.
     */
    uint8_t read8(uint8_t reg)
    {
        #ifdef TOUCH_CAPACITIVE
            if (i2cPort >= 0)
            {
                auto result = lgfx::i2c::readRegister8(i2cPort, i2cAddr, reg);
                return result.has_value() ? result.value() : 0;
            }
        #endif
        return i2c.read8(i2cAddr, reg);
    }

    /**
     * @brief Writes a single byte to a register.
     *
     * @param reg   Register address.
     * @param value Value to write.
     * @return true on success.
     */
    bool write8(uint8_t reg, uint8_t value)
    {
        #ifdef TOUCH_CAPACITIVE
            if (i2cPort >= 0)
                return lgfx::i2c::writeRegister8(i2cPort, i2cAddr, reg, value).has_value();
        #endif
        return i2c.write8(i2cAddr, reg, value);
    }

    /**
     * @brief Reads a burst of bytes starting at a register.
     *
     * @param reg    Starting register address.
     * @param buffer Destination buffer.
     * @param len    Number of bytes to read.
     */
    void readBytes(uint8_t reg, uint8_t *buffer, size_t len)
    {
        #ifdef TOUCH_CAPACITIVE
            if (i2cPort >= 0)
            {
                lgfx::i2c::readRegister(i2cPort, i2cAddr, reg, buffer, len);
                return;
            }
        #endif
        i2c.readBytes(i2cAddr, reg, buffer, len);
    }

    /**
     * @brief Reads a 16-bit value (MSB first).
     *
     * @param reg Starting register address.
     * @return 16-bit signed value.
     */
    int16_t read16(uint8_t reg)
    {
        uint8_t buf[2];
        readBytes(reg, buf, 2);
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
        readBytes(reg, buf, 2);
        return static_cast<uint16_t>((buf[1] << 8) | buf[0]);
    }

    explicit I2CDriverBase(uint8_t addr) : i2cAddr(addr) {}
};
