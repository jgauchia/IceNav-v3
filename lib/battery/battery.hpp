/**
 * @file battery.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include <cmath>
#include <esp_rom_sys.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>


/**
 * @class Battery
 * @brief Provides battery voltage monitoring and charge estimation.
 *
 * @details Handles ADC initialization, voltage range configuration, and computes battery charge percentage.
 */
class Battery
{
    private:
        float batteryMax;                    /**< Maximum (full charge) voltage. */
        float batteryMin;                    /**< Minimum (empty) voltage. */
        float lastVolt   = 0.0f;             /**< Last measured voltage (V). */
        static constexpr float V_REF = 3.3f; /**< ADC reference voltage. */

    public:
        Battery();

        void initADC();
        void setBatteryLevels(float maxVoltage, float minVoltage);
        float readBattery();
        float lastVoltage() const { return lastVolt; }
};