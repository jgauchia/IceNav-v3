/**
 * @file hal.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Hardware Abstraction Layer initialization
 * @version 0.2.9
 * @date 2026-06
 */

#include <Arduino.h>
#include "hal.hpp"
#include "i2c_espidf.hpp"
#if defined(HMC5883L) || defined(QMC5883) || defined(IMU_MPU9250)
    #include "compass.hpp"
    Compass compass;
#endif
#ifdef BME280
    #include "bme.hpp"
#endif
#ifdef ENABLE_IMU
    #include "imu.hpp"
#endif

void initHAL()
{
    #ifdef POWER_SAVE
        pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
        #ifdef ICENAV_BOARD
            gpio_hold_dis(GPIO_NUM_46);
            gpio_hold_dis((gpio_num_t)BOARD_BOOT_PIN);
            gpio_deep_sleep_hold_dis();
        #endif
    #endif
    #ifdef TDECK_ESP32S3
        pinMode(BOARD_POWERON, OUTPUT);
        digitalWrite(BOARD_POWERON, HIGH);
        pinMode(GPIO_NUM_16, INPUT);
        pinMode(SD_CS, OUTPUT);
        pinMode(RADIO_CS_PIN, OUTPUT);
        pinMode(TFT_SPI_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);
        digitalWrite(RADIO_CS_PIN, HIGH);
        digitalWrite(TFT_SPI_CS, HIGH);
        pinMode(SPI_MISO, INPUT_PULLUP);
    #endif
    i2c.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    #ifdef BME280
        initBME();
    #endif
    #if defined(HMC5883L) || defined(QMC5883) || defined(IMU_MPU9250)
        compass.init();
        vTaskDelay(pdMS_TO_TICKS(50));
    #endif
    #ifdef ENABLE_IMU
        initIMU();
    #endif
}
