/**
 * @file imu.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  IMU definition and functions - Native ESP-IDF driver
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

#ifdef MPU6050

#define ENABLE_IMU

#include "sdkconfig.h"
#include "i2c_espidf.hpp"
#include "i2c_driver_base.hpp"
#include <cstdint>

#define MPU6050_ADDRESS 0x68

class MPU6050_Driver : public I2CDriverBase
{
    private:
        float accelScale;
        float gyroScale;

        void getGyro(float &x, float &y, float &z);
        float getTemp();

        #ifdef WAVESHARE_P4_35
            int sharedI2cPort = -1;
            uint8_t readSharedReg(uint8_t reg);
            void writeSharedReg(uint8_t reg, uint8_t value);
            void readSharedBytes(uint8_t reg, uint8_t *buffer, size_t len);
        #endif

    public:
        void getAccel(float &x, float &y, float &z);
        MPU6050_Driver();
        bool begin(uint8_t addr = MPU6050_ADDRESS);
        #ifdef WAVESHARE_P4_35
            bool beginShared(int i2cPort, uint8_t addr = MPU6050_ADDRESS);
        #endif
        void setAccelRange(uint8_t range);
        void setGyroRange(uint8_t range);
        void readAll(float &ax, float &ay, float &az,
                     float &gx, float &gy, float &gz, float &temp);
};

extern MPU6050_Driver mpu;

void initIMU();

#endif
