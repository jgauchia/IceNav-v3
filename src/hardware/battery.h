/**
 * @file battery.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.1
 * @date 2022-10-09
 */
#include <driver/adc.h>

int batt_level = 0;

float battery_max = 4.2;     // maximum voltage of battery
float battery_min = 3.6;     // minimum voltage of battery before shutdown
float battery_offset = 1.31; // offset battery to full charge (divder circuit)

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
  for (int i = 0; i < 500; i++)
  {
    sum += (long)adc1_get_raw(ADC1_CHANNEL_6);
    delayMicroseconds(100);
  }
  voltage = sum / (float)500;
  voltage = adc1_get_raw(ADC1_CHANNEL_6);
  voltage = (voltage * 1.1) / 4096.0; 

#ifdef CUSTOMBOARD
  // custom board has a divider circuit
  float R1 = 100000.0; // resistance of R1 (100K)
  float R2 = 100000.0; // resistance of R2 (100K)
  voltage = voltage / (R2 / (R1 + R2));
  voltage = (voltage * battery_max) / battery_offset;
#endif

  voltage = roundf(voltage * 100) / 100;
  output = ((voltage - battery_min) / (battery_max - battery_min)) * 100;
  if (output <= 100)
    return output;
  else
    return 0.0f;
}
