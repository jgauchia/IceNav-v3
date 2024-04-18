/**
 * @file bme.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  BME280 Sensor functions
 * @version 0.1.8
 * @date 2024-04
 */

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
uint8_t tempOld = 0;
