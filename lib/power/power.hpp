/**
 * @file power.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 Power Management functions
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "i2c_espidf.hpp"
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>
#include "tft.hpp"

void closeMsg();

extern TaskHandle_t gpsTaskHandle;

extern RTC_DATA_ATTR time_t rtcSavedTime;
extern RTC_DATA_ATTR bool   rtcTimeValid;

/**
 * @class Power
 * @brief Power management class for device sleep and shutdown operations
 *
 */
class Power
{
	private:
		void powerDeepSleep();
		void powerLightSleep();
		void powerOffPeripherals();

	public:
		Power();

		void deviceSuspend();
		void deviceShutdown();
};