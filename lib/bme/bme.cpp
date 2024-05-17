/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  BME280 Sensor functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "bme.hpp"

#ifdef ENABLE_BME

Adafruit_BME280 bme = Adafruit_BME280();

/**
 * @brief Init BME sensor
 *
 */
void initBME()
{
    bme.begin(BME_ADDRESS);
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_NONE, // pressure
                    Adafruit_BME280::SAMPLING_NONE, // humidity
                    Adafruit_BME280::FILTER_OFF,
                    Adafruit_BME280::STANDBY_MS_1000);
}

#endif