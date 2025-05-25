/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

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