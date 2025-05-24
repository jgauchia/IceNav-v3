/**
 * @file imu.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  IMU definition and functions
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#pragma once

#ifdef MPU6050
#include <Adafruit_MPU6050.h>
extern Adafruit_MPU6050 mpu;
#define ENABLE_IMU
#endif

#ifdef ENABLE_IMU
void initIMU();
#endif