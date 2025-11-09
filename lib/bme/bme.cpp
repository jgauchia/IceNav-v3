/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions
 * @version 0.2.3
 * @date 2025-11
 */

#include "bme.hpp"

#ifdef BME280

Adafruit_BME280 bme = Adafruit_BME280();

uint8_t tempValue = 0; /**< Stores the current temperature value from the BME280 sensor. */
uint8_t tempOld = 0; /**< Stores the previous temperature value for comparison. */

/**
 * @brief Initializes the BME280 sensor and sets up I2C communication.
 *
 * @details Optionally allows advanced configuration for oversampling and filtering.
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
