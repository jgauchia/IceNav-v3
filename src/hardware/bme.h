/**
 * @file bme.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions
 * @version 0.1.6
 * @date 2023-06-14
 */

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
uint8_t temp_old = 0;
