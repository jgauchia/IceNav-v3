/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  BME280 Sensor functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef BME_HPP
#define BME_HPP

#ifdef BME280

#define ENABLE_TEMP

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/**
 * @brief BME280 Address
 *
 */
#define BME_ADDRESS 0x76

extern Adafruit_BME280 bme;
extern uint8_t tempValue;
extern uint8_t tempOld;

void initBME();

#endif

#endif
