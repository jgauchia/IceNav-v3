/**
 * @file powerP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 power management implementation (minimal skeleton)
 * @version 0.3.0
 * @date 2026-06
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "power.hpp"

#include "i2c_espidf.hpp"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sleep.h>
#include "tft.hpp"
#include "storage.hpp"
#include "../../../../include/hal.hpp"
#include "lvgl.h"
#include "globalGuiDef.h"
#include "taskControl.hpp"
#include <time.h>

void closeMsg();

extern Storage storage;

static const char *TAG = "POWER";

RTC_DATA_ATTR time_t rtcSavedTime = 0;
RTC_DATA_ATTR bool   rtcTimeValid = false;

/**
 * @class PowerP4
 * @brief Layer-0 power implementation for ESP32-P4 boards (skeleton).
 *
 * @details Minimal sleep/shutdown flow so the P4 environments build and degrade
 *          safely. Wakeup sources, radio (ESP-Hosted C6) teardown and the final
 *          deep-sleep tuning are implemented in a later phase; on the P4 the radio
 *          lives in the C6 co-processor, so no local esp_wifi/esp_bt teardown
 *          applies here.
 */
class PowerP4 : public IPower
{
public:
    void suspend() override
    {
        int brightness = tft.getBrightness();
        closeMsg();
        lv_refr_now(display_drv);
        tftOff();

        suspendAllTasks();

        powerLightSleep();

        resumeAllTasks();

        tftOn(brightness);
        while (gpio_get_level((gpio_num_t)BOARD_BOOT_PIN) != 1)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
        };
        ESP_LOGV(TAG, "Exited sleep mode");
    }

    void shutdown() override
    {
        powerOffPeripherals();
        powerDeepSleep();
    }

private:
    /**
     * @brief Deep Sleep Mode (skeleton).
     *
     * @details Saves RTC time and enters deep sleep. Wakeup source configuration
     *          is completed in a later phase.
     */
    void powerDeepSleep()
    {
        esp_deep_sleep_disable_rom_logging();
        vTaskDelay(pdMS_TO_TICKS(10));

        rtcSavedTime = time(NULL);
        rtcTimeValid = (rtcSavedTime > 0);

        esp_deep_sleep_start();
    }

    /**
     * @brief Light Sleep Mode (skeleton).
     */
    void powerLightSleep()
    {
        esp_light_sleep_start();
    }

    /**
     * @brief Powers off peripheral devices to reduce power consumption.
     *
     * @details Turns off the TFT display, clears the screen and ends I2C. The SD
     *          card is unmounted; on the P4 it is driven over SDMMC (added in a
     *          later phase), so no SPI bus is freed here.
     */
    void powerOffPeripherals()
    {
        tftOff();
        tft.fillScreen(TFT_BLACK);
        storage.deinitSD();
        i2c.end();
    }
};

/**
 * @brief Provides the P4 power implementation as the Layer-1 singleton.
 */
IPower &power()
{
    static PowerP4 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32P4
