/**
 * @file battery.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef BATTERY_HPP
#define BATTERY_HPP

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

class Battery
{
private:
  float batteryMax;
  float batteryMin;
  static constexpr float V_REF = 3.9; // ADC reference voltage

public:
  Battery();

  void initADC();
  void setBatteryLevels(float maxVoltage, float minVoltage);
  float readBattery();
};

#endif
