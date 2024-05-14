/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  BME280 Sensor functions
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef BME_HPP
#define BME_HPP

#ifdef CUSTOMBOARD

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

extern Adafruit_BME280 bme;

#endif

#endif
