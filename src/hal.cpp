/**
 * @file hal.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Hardware Abstraction Layer initialization
 * @version 0.3.0
 * @date 2026-06
 */

#include <Arduino.h>
#include "hal.hpp"
#include "i2c_espidf.hpp"
// TOUCH_CAPACITIVE is a #define from lgfxCommon.hpp, only visible once the
// panel header (via panelSelect.hpp) has been included in this translation
// unit — include it unconditionally so the macro resolves regardless of
// include order (see i2c_driver_base.hpp for the same pattern).
#include "panelSelect.hpp"
#ifdef POWER_SAVE
    #include <esp_pm.h>
#endif
#if defined(IMU_MPU9250) || defined(COMPASS_AUTO)
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
        #if CONFIG_IDF_TARGET_ESP32S3
            esp_pm_config_esp32s3_t pmConfig = {};
            pmConfig.max_freq_mhz       = 240;
            pmConfig.min_freq_mhz       = 40;
            pmConfig.light_sleep_enable = false;
            esp_pm_configure(&pmConfig);
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
    // On boards where the touch panel is I2C (TOUCH_CAPACITIVE), LovyanGFX
    // owns the shared I2C bus; the new i2c_master driver forbids a second
    // owner of the same port, so i2c_espidf must not create its own bus
    // here. All I2C sensor init (BME280, compass, IMU) then happens in
    // main.cpp, after initTFT(), sharing that bus instead.
    #ifndef TOUCH_CAPACITIVE
        i2c.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        #ifdef BME280
            initBME();
        #endif
        #ifdef IMU_MPU9250
            compass.init();
            vTaskDelay(pdMS_TO_TICKS(50));
        #endif
        #ifdef COMPASS_AUTO
            compass.initShared();
            vTaskDelay(pdMS_TO_TICKS(50));
        #endif
        #ifdef ENABLE_IMU
            initIMU();
        #endif
    #endif
}
