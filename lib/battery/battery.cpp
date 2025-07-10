/**
 * @file battery.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "battery.hpp"

/**
 * @brief Constructs a Battery monitoring object for voltage measurement and percentage calculation.
 */
Battery::Battery() {}

/**
 * @brief Initializes the ADC channel(s) required for battery voltage measurement.
 *
 * @details Configures the hardware ADC based on the ESP32 chip (ADC1 or ADC2).
 */
void Battery::initADC()
{
#ifdef ADC1
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATT_PIN, ADC_ATTEN_DB_12);
#endif

#ifdef ADC2
    adc2_config_channel_atten(BATT_PIN, ADC_ATTEN_DB_12);
#endif
}

/**
 * @brief Sets the maximum and minimum voltage levels for battery charge calculation.
 *
 * @param maxVoltage Voltage considered as fully charged.
 * @param minVoltage Voltage considered as minimum safe level.
 */
void Battery::setBatteryLevels(float maxVoltage, float minVoltage)
{
    batteryMax = maxVoltage;
    batteryMin = minVoltage;
}

/**
 * @brief Reads and computes the current battery charge as a percentage.
 *
 * @details Takes 100 ADC samples, averages them, compensates for the voltage divider, and calculates the charge percentage.
 *          Returns a value between 0 and 100 (values above 160 are treated as 0).
 *
 * @return float Percentage of battery charge (0–100% typically).
 */
float Battery::readBattery() 
{
    int32_t sum = 0;              /**< Accumulator for ADC samples. */
    constexpr int samples = 100;  /**< Number of ADC samples. */
    constexpr float invSamples = 1.0f / samples; /**< Precomputed inverse for averaging. */

    /**< Collect ADC samples */
    for (int i = 0; i < samples; ++i)
    {
        #ifdef ADC1
            sum += static_cast<int32_t>(adc1_get_raw(BATT_PIN));
        #endif

        #ifdef ADC2
            int readRaw;
            if (adc2_get_raw(BATT_PIN, ADC_WIDTH_BIT_12, &readRaw) == ESP_OK)
                sum += static_cast<int32_t>(readRaw);
        #endif

        ets_delay_us(100); /**< Slightly reduced delay for better responsiveness. */
    }

    /**< Convert ADC value to voltage */
    float voltage = (sum * invSamples) * (V_REF / 4096.0f); /**< Averaged and scaled ADC voltage. */

    /**< Compensate for voltage divider (R1 = R2 = 100kΩ) */
    voltage *= 2.0f;

    /**< Round to 2 decimal places */
    voltage = roundf(voltage * 100.0f) * 0.01f;

    /**< Calculate battery percentage */
    float output = ((voltage - batteryMin) / (batteryMax - batteryMin)) * 100.0f;

    /**< Clamp out-of-range values */
    return (output <= 160.0f) ? output : 0.0f;
}
