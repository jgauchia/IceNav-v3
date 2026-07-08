/**
 * @file axp2101.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  AXP2101 PMIC battery monitor definition and functions (Waveshare P4-3.5)
 * @version 0.3.0
 * @date 2026-07
 */

#pragma once

#include <stdint.h>

/**
 * @class Axp2101
 * @brief Reads battery voltage, charge percentage and charging status from the AXP2101 PMIC.
 *
 * @details Shares the I2C bus already owned by LovyanGFX (touch controller) on
 *          the Waveshare P4-3.5 (WAVESHARE_P4_35), talking to it through
 *          lgfx::i2c on the same port instead of opening a second bus.
 *          Registers per the X-Powers AXP2101 datasheet.
 */
class Axp2101
{
    private:
        int i2cPort = -1;
        bool inited = false;
        float lastVolt = 0.0f;

        static constexpr uint8_t I2C_ADDR       = 0x34;
        static constexpr uint8_t REG_STATUS1    = 0x00;   // bit3: battery present
        static constexpr uint8_t REG_STATUS2    = 0x01;   // bit5: battery is charging, bit0: VBUS available
        static constexpr uint8_t REG_ADC_ENABLE = 0x30;   // bit5: enable VBAT measurement
        static constexpr uint8_t REG_VBAT_H     = 0x34;   // 12-bit VBAT, left-aligned, 1 LSB ~= 1.1mV
        static constexpr uint8_t REG_VBAT_L     = 0x35;
        static constexpr uint8_t REG_SOC        = 0xA4;   // fuel gauge charge percentage (0-100)

    public:
        void begin(int i2cPort);
        float readBattery();
        bool isCharging();
        float lastVoltage() const { return lastVolt; }
};
