/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions - Native ESP-IDF driver
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#ifdef BME280

#define ENABLE_TEMP

#include "i2c_espidf.hpp"
#include <cstdint>

#define BME_ADDRESS 0x76

class BME280_Driver
{
    private:
        uint8_t i2cAddr;

        uint16_t dig_T1;
        int16_t dig_T2;
        int16_t dig_T3;

        uint16_t dig_P1;
        int16_t dig_P2;
        int16_t dig_P3;
        int16_t dig_P4;
        int16_t dig_P5;
        int16_t dig_P6;
        int16_t dig_P7;
        int16_t dig_P8;
        int16_t dig_P9;

        uint8_t dig_H1;
        int16_t dig_H2;
        uint8_t dig_H3;
        int16_t dig_H4;
        int16_t dig_H5;
        int8_t dig_H6;

        int32_t t_fine;

        uint8_t read8(uint8_t reg);
        uint16_t read16_LE(uint8_t reg);
        int16_t readS16_LE(uint8_t reg);
        void write8(uint8_t reg, uint8_t value);
        void readCoefficients();

    public:
        BME280_Driver();
        bool begin(uint8_t addr = BME_ADDRESS);
        float readTemperature();
        float readPressure();
        float readHumidity();
        float readAltitude(float seaLevelPressure = 101325.0f);
};

extern BME280_Driver bme;
extern uint8_t tempValue;
extern uint8_t tempOld;

void initBME();

#endif
