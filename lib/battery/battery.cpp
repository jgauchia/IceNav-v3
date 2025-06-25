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
 * Configures the hardware ADC based on the ESP32 chip (ADC1 or ADC2).
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
 * Takes 100 ADC samples, averages them, compensates for the voltage divider, and calculates the charge percentage.
 * Returns a value between 0 and 100 (values above 160 are treated as 0).
 *
 * @return float Percentage of battery charge (0–100% typically).
 */
float Battery::readBattery() 
{

    long sum = 0;        /**< Sum of samples taken. */
    float voltage = 0.0; /**< Calculated voltage. */
    float output = 0.0;  /**< Output value. */


    for (int i = 0; i < 100; i++)
    {
        #ifdef ADC1
            sum += static_cast<long>(adc1_get_raw(BATT_PIN));
        #endif

        #ifdef ADC2
            int readRaw;
            esp_err_t r = adc2_get_raw(BATT_PIN, ADC_WIDTH_BIT_12, &readRaw);
            if (r == ESP_OK)
                sum += static_cast<long>(readRaw);
        #endif

        delayMicroseconds(150);
    }

    voltage = sum / 100.0;
    /**< Custom board has a divider circuit */
    constexpr float R1 = 100000.0; /**< Resistance of R1 (100K) */
    constexpr float R2 = 100000.0; /**< Resistance of R2 (100K) */
    voltage = (voltage * V_REF) / 4096.0;
    voltage = voltage / (R2 / (R1 + R2));
    voltage = roundf(voltage * 100) / 100;

    output = ((voltage - batteryMin) / (batteryMax - batteryMin)) * 100;
    return (output <= 160) ? output : 0.0f;
}
