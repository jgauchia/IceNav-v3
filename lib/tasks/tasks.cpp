/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.2.3
 * @date 2025-06
 */

#include "tasks.hpp"

TaskHandle_t LVGLTaskHandler;      /**< Handle for the LVGL task */
xSemaphoreHandle gpsMutex;         /**< Mutex for GPS resource protection */
extern Gps gps;

static const char* TAG PROGMEM = "Task";

/**
 * @brief Read GPS data
 *
 * Task function that reads data from the GPS module, optionally prints NMEA output,
 * and updates the GPS fix structure. Uses the gpsMutex for thread safety.
 *
 * @param pvParameters Task parameters (unused)
 */
void gpsTask(void *pvParameters)
{
	ESP_LOGV(TAG, "GPS Task - running on core %d", xPortGetCoreID());
	ESP_LOGV(TAG, "Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
	while (1)
	{
		if ( xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE )
		{
			if (nmea_output_enable)
			{
				while (gpsPort.available())
				{
					char c = gpsPort.read();
					Serial.print(c);
				}
			} 

			while (GPS.available( gpsPort )) 
			{
				fix = GPS.read();
				gps.getGPSData();
			}

			xSemaphoreGive(gpsMutex);

			vTaskDelay(1); /// portTICK_PERIOD_MS);
		}
	}
}

/**
 * @brief Init GPS task
 *
 * Creates and starts the GPS task 
 */
void initGpsTask()
{
	xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 8192, NULL, 1, NULL, 0);
	delay(500);
}

/**
 * @brief CLI task
 *
 * Task function for the command-line interface. 
 *
 * @param param Task parameters (unused)
 */
#ifndef DISABLE_CLI
void cliTask(void *param) 
{
	ESP_LOGV(TAG, "CLI Task - running on core %d", xPortGetCoreID());
	ESP_LOGV(TAG, "Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
	while(1) 
	{
		wcli.loop();
		vTaskDelay(60 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

/**
 * @brief Init CLI task
 *
 * Creates and starts CLI task
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 20000, NULL, 1, NULL, 1); }

#endif


