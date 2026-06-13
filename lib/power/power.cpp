/**
 * @file power.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 Power Management functions
 * @version 0.2.9
 * @date 2026-06
 */

#include "power.hpp"

#include "storage.hpp"
#include "../../include/hal.hpp"
#include "lvgl.h"
#include "globalGuiDef.h"
#include <time.h>
extern Storage storage;

static const char *TAG = "Power";

RTC_DATA_ATTR time_t rtcSavedTime = 0;
RTC_DATA_ATTR bool   rtcTimeValid = false;

/**
 * @brief Power Class constructor
 *
 * @details Initializes the Power class and optionally disables radio interfaces (WiFi and Bluetooth)
 * 			if the DISABLE_RADIO macro is defined, to reduce power consumption.
 */
Power::Power()
{
    #ifdef DISABLE_RADIO
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_bt_controller_disable();
    #endif
}

/**
 * @brief Deep Sleep Mode
 *
 * @details Puts the device into deep sleep mode to minimize power consumption.
 * 			Disables Bluetooth and WiFi, configures wakeup sources, and starts deep sleep.
 */
void Power::powerDeepSleep()
{
    esp_bluedroid_disable();
    if (esp_bt_controller_disable() != ESP_OK)
        ESP_LOGE(TAG, "Failed to disable BT controller");
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
void Power::powerLightSleep()
{
    esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_light_sleep_start();
}

/**
 * @brief Powers off peripheral devices to reduce power consumption.
 *
 * @details Turns off the TFT display, clears the screen, and ends SPI and I2C communications.
 */
void Power::powerOffPeripherals()
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

/**
 * @brief Core light suspend and TFT off
 *
 * @details Suspends the device by turning off the TFT display, entering light sleep mode,
 * 			and restoring the display brightness upon wakeup. Ensures the device only resumes
 * 			when the boot pin is released.
 */
void Power::deviceSuspend()
{
    int brightness = tft.getBrightness();
    closeMsg();
    lv_refr_now(display);
    tftOff();

    if (gpsTaskHandle != NULL)
        vTaskSuspend(gpsTaskHandle);

    powerLightSleep();

    if (gpsTaskHandle != NULL)
        vTaskResume(gpsTaskHandle);

    tftOn(brightness);
    while (gpio_get_level((gpio_num_t)BOARD_BOOT_PIN) != 1)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
    };
    log_v("Exited sleep mode");
}

/**
 * @brief Powers off peripherals and enters deep sleep mode.
 *
 * @details Turns off all peripheral devices and puts the device into deep sleep to minimize power consumption.
 */
void Power::deviceShutdown()
{
    powerOffPeripherals();
    powerDeepSleep();
}
