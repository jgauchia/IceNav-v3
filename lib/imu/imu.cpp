/**
 * @file imu.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  IMU definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

 #include "imu.hpp"

#ifdef MPU6050
Adafruit_MPU6050 mpu;
#endif

#ifdef ENABLE_IMU

static const char* TAG PROGMEM = "IMU";
/**
 * @brief Init IMU
 *
 */
void initIMU()
{
  if (!mpu.begin()) 
    ESP_LOGE(TAG, "Failed to init IMU");
  else
    ESP_LOGI(TAG, "IMU init");
}

#endif