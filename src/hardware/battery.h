/**
 * @file battery.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.1.2
 * @date 2023-04-15
 */

#include <driver/adc.h>
#include <esp_adc_cal.h>

uint8_t batt_level = 0;
uint8_t batt_level_old = 0;

esp_adc_cal_characteristics_t characteristics;
#define V_REF 3.9 // ADC reference voltage

float battery_max = 4.20;     // 4.2;      // maximum voltage of battery
float battery_min = 3.40;     // 3.6;      // minimum voltage of battery before shutdown

/**
 * @brief Configurate ADC Channel for battery reading
 *
 */
void init_ADC()
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
float battery_read()
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
  output = ((voltage - battery_min) / (battery_max - battery_min)) * 100;
  if (output <= 160)
    return output;
  else
    return 0.0f;
}
