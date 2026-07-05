/**
 * @file powerS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 power management implementation (esp_sleep + light/deep sleep)
 * @version 0.3.0
 * @date 2026-06
 */

#include "power.hpp"

#include "i2c_espidf.hpp"
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
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
 * @class PowerS3
 * @brief Layer-0 power implementation for ESP32-S3 boards.
 *
 * @details Wraps the esp_sleep light/deep sleep flow and the peripheral
 *          power-off sequence. Radio is disabled at construction when the
 *          DISABLE_RADIO macro is defined, to reduce power consumption.
 */
class PowerS3 : public IPower
{
public:
    PowerS3()
    {
        #ifdef DISABLE_RADIO
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_wifi_deinit();
        #endif
    }

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
     * @brief Deep Sleep Mode
     *
     * @details Puts the device into deep sleep mode to minimize power consumption.
     * 			Disables WiFi, configures wakeup sources, and starts deep sleep.
     */
    void powerDeepSleep()
    {
        if (esp_wifi_stop() != ESP_OK)
            ESP_LOGE(TAG, "Failed to stop WiFi");
        esp_deep_sleep_disable_rom_logging();
        vTaskDelay(pdMS_TO_TICKS(10));

        #ifdef ICENAV_BOARD
            // If you need other peripherals to maintain power, please set the IO port to hold
            gpio_hold_en(GPIO_NUM_46);
            gpio_hold_en((gpio_num_t)BOARD_BOOT_PIN);
            gpio_deep_sleep_hold_en();
        #endif

        rtcSavedTime = time(NULL);
        rtcTimeValid = (rtcSavedTime > 0);

        esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
        esp_deep_sleep_start();
    }

    /**
     * @brief Sleep Mode
     *
     * @details Puts the device into light sleep mode until an external event occurs.
     * 			Configures the device to wake up on a low signal from the boot pin.
     */
    void powerLightSleep()
    {
        esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
        esp_light_sleep_start();
    }

    /**
     * @brief Powers off peripheral devices to reduce power consumption.
     *
     * @details Turns off the TFT display, clears the screen, and ends SPI and I2C communications.
     */
    void powerOffPeripherals()
    {
        tftOff();
        tft.fillScreen(TFT_BLACK);

        // Properly deinitialize SD card before freeing SPI bus (only if not SPI_SHARED)
        #ifndef SPI_SHARED
            storage.deinitSD();
            if (spi_bus_free(SPI2_HOST) != ESP_OK)
                ESP_LOGE(TAG, "Failed to free SPI bus");
        #endif
        i2c.end();
    }
};

/**
 * @brief Provides the S3 power implementation as the Layer-1 singleton.
 */
IPower &power()
{
    static PowerS3 instance;
    return instance;
}
