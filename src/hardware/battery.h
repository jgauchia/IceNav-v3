/**
 * @file battery.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery monitor definition and functions
 * @version 0.1
 * @date 2022-10-09
 */

#include <Battery18650Stats.h>

#define CONVERSION_FACTOR 1.81
#define READS 50
Battery18650Stats batt(ADC_BATT_PIN, CONVERSION_FACTOR, READS);
int batt_level = 0;

/**
 * @brief Battery read delay
 *
 */
#define BATT_UPDATE_TIME 1000
MyDelay BATTtime(BATT_UPDATE_TIME);

/**
 * @brief Read battery level
 * 
 * @return int ->battery level
 */
int Read_Battery()
{
  return batt.getBatteryChargeLevel(true);
}