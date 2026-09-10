/**
 * @file axp2101.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  AXP2101 PMIC battery monitor definition and functions (Waveshare P4-3.5)
 * @version 0.3.0
 * @date 2026-07
 */

#include "axp2101.hpp"
#include <LovyanGFX.hpp>

// Global AXP2101 instance
Axp2101 axp2101;

/**
 * @brief Attaches the driver to the I2C bus already owned by LovyanGFX.
 *
 * @details Does not create a new I2C bus: the touch controller on P4 boards
 *          already owns the shared port via lgfx::i2c, so the PMIC is reached
 *          through the same public lgfx::i2c transaction helpers instead of a
 *          second bus. Enables the VBAT ADC on the PMIC.
 *
 * @param i2cPort I2C port already initialized by LovyanGFX (touch bus).
 */
void Axp2101::begin(int i2cPort)
{
    this->i2cPort = i2cPort;
    inited = lgfx::i2c::writeRegister8(i2cPort, I2C_ADDR, REG_ADC_ENABLE, 0x20, 0x20).has_value();
}

/**
 * @brief Reads and computes the current battery charge as a percentage from the PMIC fuel gauge.
 *
 * @details Also updates the last measured battery voltage from the VBAT ADC registers.
 *
 * @return float Percentage of battery charge (0-100%), or 0 if the PMIC is not reachable.
 */
float Axp2101::readBattery()
{
    if (!inited)
        return 0.0f;

    uint8_t vbatH = 0;
    uint8_t vbatL = 0;
    auto h = lgfx::i2c::readRegister8(i2cPort, I2C_ADDR, REG_VBAT_H);
    auto l = lgfx::i2c::readRegister8(i2cPort, I2C_ADDR, REG_VBAT_L);
    if (h.has_value() && l.has_value())
    {
        vbatH = h.value();
        vbatL = l.value();
        uint16_t raw = ((uint16_t)vbatH << 8 | vbatL) & 0x3FFF;
        lastVolt = raw * 1.1f / 1000.0f;
    }

    auto soc = lgfx::i2c::readRegister8(i2cPort, I2C_ADDR, REG_SOC);
    if (!soc.has_value())
        return 0.0f;

    uint8_t percent = soc.value();
    return (percent <= 100) ? (float)percent : 100.0f;
}

/**
 * @brief Reads the real charging/power status from the PMIC.
 *
 * @details True if the PMIC reports the battery is actively charging, or if
 *          external power (VBUS) is present without a battery installed —
 *          the "battery is charging" bit never sets without a battery to
 *          charge, but the charging icon should still show while plugged in.
 *
 * @return true if the battery is charging, or VBUS is present with no battery.
 */
bool Axp2101::isCharging()
{
    if (!inited)
        return false;

    auto status2 = lgfx::i2c::readRegister8(i2cPort, I2C_ADDR, REG_STATUS2);
    if (!status2.has_value())
        return false;

    bool batteryCharging = (status2.value() & 0x20) != 0;
    bool vbusPresent     = (status2.value() & 0x01) != 0;

    auto status1 = lgfx::i2c::readRegister8(i2cPort, I2C_ADDR, REG_STATUS1);
    bool batteryPresent = status1.has_value() && (status1.value() & 0x08) != 0;

    return batteryCharging || (vbusPresent && !batteryPresent);
}

/**
 * @brief Cuts all PMIC rails except RTCLDO (soft power-off).
 *
 * @details Writes REG10H bit0 ("Soft PWROFF", self-clearing). The PMIC then
 *          drops PWROK/ESP_EN and the ESP32-P4 loses power entirely — this
 *          call does not return on real hardware. Power-on afterwards is
 *          handled autonomously by the PMIC when PWRON (the physical K1
 *          button) is pressed, per the AXP2101's default POK power-on source
 *          (datasheet 6.5.4.2); no further register configuration is needed.
 */
void Axp2101::softPowerOff()
{
    if (!inited)
        return;

    lgfx::i2c::writeRegister8(i2cPort, I2C_ADDR, REG_PMU_CONFIG, 0x01, 0x01);
}
