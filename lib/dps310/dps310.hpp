/**
 * @file dps310.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  DPS310 Sensor functions - Native ESP-IDF driver
 * @version 0.3.3
 * @date 2026-10
 */

#pragma once

#ifdef DPS310

#define ENABLE_TEMP

#include "sdkconfig.h"
#include "i2c_espidf.hpp"
#include "i2c_driver_base.hpp"
#include <cstdint>

// I2C addresses: 0x77 with SDO pulled high (most breakouts), 0x76 with SDO low.
// begin()/beginShared() auto-probe both by default; pass an address explicitly
// to probe that one only.
#define DPS310_ADDRESS  0x77
#define DPS310_ADDR_ALT 0x76

class DPS310_Driver : public I2CDriverBase
{
    private:
        int32_t c0;
        int32_t c1;
        int32_t c00;
        int32_t c10;
        int32_t c01;
        int32_t c11;
        int32_t c20;
        int32_t c21;
        int32_t c30;

        float kP;   /**< Pressure compensation scale factor (kP from datasheet Table 9). */
        float kT;   /**< Temperature compensation scale factor (kT from datasheet Table 9). */

        bool detected; /**< True once the sensor responded to initialization. */

        bool init();
        bool probe(uint8_t addr);
        void readCoefficients();
        bool forceTemp(int32_t &rawT);
        void compensate(float &temp, float &pres);

    public:
        DPS310_Driver();
        bool begin(uint8_t addr = 0);
        #ifdef TOUCH_CAPACITIVE
            bool beginShared(int i2cPort, uint8_t addr = 0);
        #endif
        uint8_t getAddress() const { return i2cAddr; }
        float readAltitude(float pressure);
        bool isDetected() const { return detected; }
        void readAll(float &temp, float &pres, float &humi);
};

extern DPS310_Driver dps310;

void initDPS310();

#endif
