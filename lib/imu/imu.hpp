/**
 * @file imu.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  IMU definition and functions - Native ESP-IDF driver
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#ifdef MPU6050

#define ENABLE_IMU

#include "i2c_espidf.hpp"
#include <cstdint>

#define MPU6050_ADDRESS 0x68

class MPU6050_Driver
{
    private:
        uint8_t i2cAddr;
        float accelScale;
        float gyroScale;

        uint8_t read8(uint8_t reg);
        void write8(uint8_t reg, uint8_t value);
        int16_t read16(uint8_t reg);

    public:
        MPU6050_Driver();
        bool begin(uint8_t addr = MPU6050_ADDRESS);
        void setAccelRange(uint8_t range);
        void setGyroRange(uint8_t range);
        void getAccel(float &x, float &y, float &z);
        void getGyro(float &x, float &y, float &z);
        float getTemp();
        void readAll(float &ax, float &ay, float &az,
                     float &gx, float &gy, float &gz, float &temp);
};

extern MPU6050_Driver mpu;

void initIMU();

#endif
