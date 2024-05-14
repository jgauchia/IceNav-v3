/**
 * @file battery.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Battery monitor definition and functions
 * @version 0.1.8
 * @date 2024-05
 */


#include "battery.hpp"

/**
 * @brief Configurate ADC Channel for battery reading
 *
 */
void initADC()
{
  // When VDD_A is 3.3V:
  //     0dB attenuaton (ADC_ATTEN_DB_0) gives full-scale voltage 1.1V
  //     2.5dB attenuation (ADC_ATTEN_DB_2_5) gives full-scale voltage 1.5V
  //     6dB attenuation (ADC_ATTEN_DB_6) gives full-scale voltage 2.2V
  //     11dB attenuation (ADC_ATTEN_DB_11) gives full-scale voltage 3.9V
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
}

/**
 * @brief Read battery charge and return %
 *
 * @return float -> % Charge
 */
float batteryRead()
{
  long sum = 0;        // sum of samples taken
  float voltage = 0.0; // calculated voltage
  float output = 0.0;  // output value
  for (int i = 0; i < 100; i++)
  {
    sum += (long)adc1_get_raw(ADC1_CHANNEL_6);
    delayMicroseconds(150);
  }
  voltage = sum / (float)100;
  // custom board has a divider circuit
  float R1 = 100000.0; // resistance of R1 (100K)
  float R2 = 100000.0; // resistance of R2 (100K)
  voltage = (voltage * V_REF) / 4096.0;
  voltage = voltage / (R2 / (R1 + R2));
  voltage = roundf(voltage * 100) / 100;
  output = ((voltage - batteryMin) / (batteryMax - batteryMin)) * 100;
  if (output <= 160)
    return output;
  else
    return 0.0f;
}