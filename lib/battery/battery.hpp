/**
 * @file battery.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#include <cmath>
#include <esp_rom_sys.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>


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

        adc_oneshot_unit_handle_t adcHandle = nullptr; /**< ADC oneshot unit handle. */
        adc_cali_handle_t caliHandle = nullptr;        /**< ADC calibration handle. */
        bool caliEnabled = false;                      /**< Whether calibration is available. */

        float lastMeanRaw = 0.0f;            /**< Previous raw ADC average for charging inference. */
        bool chargingInferred = false;       /**< Whether battery voltage is trending up (charging). */

    public:
        Battery();

        void initADC();
        void setBatteryLevels(float maxVoltage, float minVoltage);
        float readBattery();
        float lastVoltage() const { return lastVolt; }
        float maxVoltage() const  { return batteryMax; }
        bool isChargingInferred() const { return chargingInferred; } /**< Whether raw ADC trend or voltage ≥ max suggests charging. */
};