/**
 * @file imu.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  IMU definition and functions
 * @version 0.2.0_alpha
 * @date 2024-12
 */

 #include "imu.hpp"

#ifdef MPU6050
Adafruit_MPU6050 mpu;
#endif

#ifdef ENABLE_IMU

/**
 * @brief Init IMU
 *
 */
void initIMU()
{
    if (!mpu.begin()) 
        log_e("Failed to init IMU");
    else
        log_i("IMU init");
}

#endif