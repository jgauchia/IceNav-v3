/**
 * @file battery.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.3.0
 * @date 2026-06
 */

#include "battery.hpp"

// Global Battery instance
Battery battery;

/**
 * @brief Constructs a Battery monitoring object for voltage measurement and percentage calculation.
 */
Battery::Battery() : batteryMax(0.0f), batteryMin(0.0f)
{
}

/**
 * @brief Initializes the ADC channel required for battery voltage measurement.
 *
 * @details Configures the oneshot ADC unit and channel selected by BATT_ADC_UNIT and
 *          BATT_ADC_CHANNEL, with 12 dB attenuation and 12 bit width, and sets up
 *          curve-fitting calibration when available.
 */
void Battery::initADC()
{
    #ifdef BATT_ADC_UNIT
        const adc_unit_t unit = (BATT_ADC_UNIT == 2) ? ADC_UNIT_2 : ADC_UNIT_1;
        const adc_channel_t channel = (adc_channel_t)BATT_ADC_CHANNEL;

        adc_oneshot_unit_init_cfg_t unitCfg = {};
        unitCfg.unit_id = unit;
        adc_oneshot_new_unit(&unitCfg, &adcHandle);

        adc_oneshot_chan_cfg_t chanCfg = {};
        chanCfg.atten = ADC_ATTEN_DB_12;
        chanCfg.bitwidth = ADC_BITWIDTH_12;
        adc_oneshot_config_channel(adcHandle, channel, &chanCfg);

        adc_cali_curve_fitting_config_t caliCfg = {};
        caliCfg.unit_id = unit;
        caliCfg.chan = channel;
        caliCfg.atten = ADC_ATTEN_DB_12;
        caliCfg.bitwidth = ADC_BITWIDTH_12;
        caliEnabled = (adc_cali_create_scheme_curve_fitting(&caliCfg, &caliHandle) == ESP_OK);
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
    long sum = 0;        // Sum of samples taken
    float voltage = 0.0; // Calculated voltage
    float output = 0.0;  // Output value

    static constexpr int ADC_SAMPLES = 16;

    #ifdef BATT_ADC_UNIT
        const adc_channel_t channel = (adc_channel_t)BATT_ADC_CHANNEL;
        for (int i = 0; i < ADC_SAMPLES; i++)
        {
            int readRaw = 0;
            if (adc_oneshot_read(adcHandle, channel, &readRaw) == ESP_OK)
                sum += (long)readRaw;
            esp_rom_delay_us(150);
        }
    #endif

    float meanRaw = sum / (float)ADC_SAMPLES;
    // Custom board has a divider circuit
    constexpr float R1 = 100000.0f; // Resistance of R1 (100K)
    constexpr float R2 = 100000.0f; // Resistance of R2 (100K)

    #ifdef BATT_ADC_UNIT
        int caliMv = 0;
        if (caliEnabled && adc_cali_raw_to_voltage(caliHandle, (int)lroundf(meanRaw), &caliMv) == ESP_OK)
            voltage = caliMv / 1000.0f;
        else
            voltage = (meanRaw * V_REF) / 4096.0f;
    #else
        voltage = (meanRaw * V_REF) / 4096.0f;
    #endif

    voltage = voltage * ((R1 + R2) / R2);
    voltage = roundf(voltage * 100.0f) / 100.0f;
    lastVolt = voltage;
    output = ((voltage - batteryMin) / (batteryMax - batteryMin)) * 100.0f;

    return (output <= 500) ? output : 0.0f;
}
        