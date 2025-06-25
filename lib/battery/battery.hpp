/**
 * @file battery.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>


/**
 * @class Battery
 * @brief Provides battery voltage monitoring and charge estimation.
 *
 * Handles ADC initialization, voltage range configuration, and computes battery charge percentage.
 */
class Battery
{
private:
	float batteryMax; 					/**< Maximum (full charge) voltage. */
	float batteryMin;					/**< Minimum (empty) voltage. */
	static constexpr float V_REF = 3.9; /**< ADC reference voltage. */

public:
  	Battery();

	void initADC();
	void setBatteryLevels(float maxVoltage, float minVoltage);
	float readBattery();
};