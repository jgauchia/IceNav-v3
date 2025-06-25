/**
 * @file power.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 Power Management functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "power.hpp"

extern const uint8_t BOARD_BOOT_PIN; /**< External declaration for the board's boot pin number. */

/**
 * @brief Power Class constructor
 *
 * Initializes the Power class and optionally disables radio interfaces (WiFi and Bluetooth)
 * if the DISABLE_RADIO macro is defined, to reduce power consumption.
 */
Power::Power()
{
#ifdef DISABLE_RADIO
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);
	btStop();
	esp_wifi_stop();
	esp_bt_controller_disable();
#endif
}

/**
 * @brief Deep Sleep Mode
 *
 * Puts the device into deep sleep mode to minimize power consumption.
 * Disables Bluetooth and WiFi, configures wakeup sources, and starts deep sleep.
 */
void Power::powerDeepSleep()
{
	esp_bluedroid_disable();
	esp_bt_controller_disable();
	esp_wifi_stop();
	esp_deep_sleep_disable_rom_logging();
	delay(10);

	#ifdef ICENAV_BOARD
		// If you need other peripherals to maintain power, please set the IO port to hold
		gpio_hold_en(GPIO_NUM_46);
		gpio_hold_en((gpio_num_t)BOARD_BOOT_PIN);
		gpio_deep_sleep_hold_en();
	#endif

	esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
	esp_deep_sleep_start();
}

/**
 * @brief Sleep Mode Timer
 *
 * Puts the device into light sleep mode for a specified duration.
 * Enables timer wakeup and starts light sleep.
 *
 * @param millis Duration of light sleep in milliseconds
 */
void Power::powerLightSleepTimer(int millis)
{
	esp_sleep_enable_timer_wakeup(millis * 1000);
	esp_err_t rtc_gpio_hold_en(gpio_num_t GPIO_NUM_5);
	esp_light_sleep_start();
}

/**
 * @brief Sleep Mode
 *
 * Puts the device into light sleep mode until an external event occurs.
 * Configures the device to wake up on a low signal from the boot pin.
 */
void Power::powerLightSleep()
{
	esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
	esp_light_sleep_start();
}

/**
 * @brief Powers off peripheral devices to reduce power consumption.
 *
 * Turns off the TFT display, clears the screen, and ends SPI and I2C communications.
 */
void Power::powerOffPeripherals()
{
	tftOff();
	tft.fillScreen(TFT_BLACK);
	SPI.end();
	Wire.end();
}

/**
 * @brief Core light suspend and TFT off
 *
 * Suspends the device by turning off the TFT display, entering light sleep mode,
 * and restoring the display brightness upon wakeup. Ensures the device only resumes
 * when the boot pin is released.
 */
void Power::deviceSuspend()
{
	int brightness = tft.getBrightness();
	lv_msgbox_close(powerMsg); 
	lv_refr_now(display);
	tftOff();
	powerLightSleep();
	tftOn(brightness);
	while (digitalRead(BOARD_BOOT_PIN) != 1)
	{ 
		delay(5);
	};
	log_v("Exited sleep mode");
}

/**
 * @brief Powers off peripherals and enters deep sleep mode.
 *
 * Turns off all peripheral devices and puts the device into deep sleep to minimize power consumption.
 */
void Power::deviceShutdown()
{
	powerOffPeripherals();
	powerDeepSleep();
}
