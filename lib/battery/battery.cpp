/**
 * @file battery.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Battery monitor definition and functions
 * @version 0.1.8
 * @date 2024-11
 */


#include "battery.hpp"

/**
 * @brief Battery values
 *
 */
uint8_t battLevel = 0;
uint8_t battLevelOld = 0;
float batteryMax = 4.2;      // maximum voltage of battery
float batteryMin = 3.6;      // minimum voltage of battery before shutdown

/**
 * @brief Configure ADC Channel for battery reading
 *
 */
void initADC()
{
  // When VDD_A is 3.3V:
  //     0dB attenuation (ADC_ATTEN_DB_0) gives full-scale voltage 1.1V
  //     2.5dB attenuation (ADC_ATTEN_DB_2_5) gives full-scale voltage 1.5V
  //     6dB attenuation (ADC_ATTEN_DB_6) gives full-scale voltage 2.2V
  //     12dB attenuation (ADC_ATTEN_DB_12) gives full-scale voltage 3.9V

  #ifdef ADC1
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATT_PIN, ADC_ATTEN_DB_12); 
  #endif

  #ifdef ADC2
    adc2_config_channel_atten(BATT_PIN, ADC_ATTEN_DB_12);
  #endif
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

    #ifdef ADC1
      sum += (long)adc1_get_raw(BATT_PIN);
    #endif

    #ifdef ADC2
     int readRaw;
     esp_err_t r = adc2_get_raw(BATT_PIN, ADC_WIDTH_BIT_12, &readRaw);
     if (r == ESP_OK)
      sum += (long)readRaw;
    #endif

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
