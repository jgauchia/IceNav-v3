/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions
 * @version 0.2.1
 * @date 2025-05
 */

#include "bme.hpp"

#ifdef BME280

Adafruit_BME280 bme = Adafruit_BME280();

/**
 * @brief Temperature reading values
 *
 */
uint8_t tempValue = 0;
uint8_t tempOld = 0;

/**
 * @brief Init BME sensor
 *
 */
void initBME()
{
  bme.begin(BME_ADDRESS);
  // bme.setSampling(Adafruit_BME280::MODE_NORMAL,
  //                 Adafruit_BME280::SAMPLING_X8,   // temperature
  //                 Adafruit_BME280::SAMPLING_NONE, // pressure
  //                 Adafruit_BME280::SAMPLING_NONE, // humidity
  //                 Adafruit_BME280::FILTER_OFF,
  //                 Adafruit_BME280::STANDBY_MS_1000);
}

#endif
