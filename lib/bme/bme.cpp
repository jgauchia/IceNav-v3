/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  BME280 Sensor functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "bme.hpp"

#ifdef CUSTOMBOARD

Adafruit_BME280 bme = Adafruit_BME280();

#endif