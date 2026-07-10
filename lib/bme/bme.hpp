/**
 * @file bme.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions - Native ESP-IDF driver
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#ifdef BME280

#define ENABLE_TEMP

#include "sdkconfig.h"
#include "i2c_espidf.hpp"
#include "i2c_driver_base.hpp"
#include <cstdint>

#define BME_ADDRESS 0x76

class BME280_Driver : public I2CDriverBase
{
    private:
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

        void readCoefficients();

        int32_t readRawT();
        int32_t readRawP();
        int32_t readRawH();
        float compensateTemperature(int32_t adc_T);
        float compensatePressure(int32_t adc_P);
        float compensateHumidity(int32_t adc_H);

        float readTemperature();
        float readPressure();
        float readHumidity();

        #ifdef WAVESHARE_P4_35
            int sharedI2cPort = -1;
            uint8_t readSharedReg(uint8_t reg);
            void writeSharedReg(uint8_t reg, uint8_t value);
            void readSharedBytes(uint8_t reg, uint8_t *buffer, size_t len);
        #endif

    public:
        BME280_Driver();
        bool begin(uint8_t addr = BME_ADDRESS);
        #ifdef WAVESHARE_P4_35
            bool beginShared(int i2cPort, uint8_t addr = BME_ADDRESS);
        #endif
        float readAltitude(float pressure);
        void readAll(float &temp, float &pres, float &humi);
};

extern BME280_Driver bme;

void initBME();

#endif
