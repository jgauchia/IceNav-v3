/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#ifdef BME280

#define ENABLE_TEMP

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_ADDRESS 0x76          /**< I2C address for the BME280 sensor (default: 0x76). */
extern Adafruit_BME280 bme;       /**< Global instance of the BME280 sensor driver. */
extern uint8_t tempValue;         /**< Stores the latest temperature reading from the BME280 sensor. */
extern uint8_t tempOld;           /**< Stores the previous temperature value to detect changes. */

void initBME();

#endif