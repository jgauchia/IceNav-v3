/**
 * @file tasks.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks implementation for GPS and CLI management
 * @version 0.2.3
 * @date 2025-11
 * @details This file contains the implementation of FreeRTOS tasks for GPS data processing
 *          and CLI interface management. It handles thread-safe GPS data reading and
 *          command-line interface operations with proper mutex protection.
 */

#include "tasks.hpp"

TaskHandle_t LVGLTaskHandler;      /**< Handle for the LVGL task */
xSemaphoreHandle gpsMutex;         /**< Mutex for GPS resource protection */
extern Gps gps;                    /**< Global GPS instance for data processing */

static const char* TAG PROGMEM = "Task"; /**< Logging tag for task operations */

/**
 * @brief GPS data processing task
 *
 * @details Continuously reads GPS data from the serial port, processes NMEA sentences,
 *          and updates the global GPS fix structure. Handles optional NMEA output to
 *          serial console and ensures thread-safe access using gpsMutex. The task runs
 *          on core 0 with high priority to ensure real-time GPS data processing.
 *
 * @param pvParameters Task parameters (unused in current implementation)
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
 * @brief Initialize GPS processing task
 *
 * @details Creates and starts the GPS task on core 0 with 8KB stack size and priority 1.
 *          Includes a 500ms delay after task creation to ensure proper initialization
 *          before other system components attempt to access GPS data.
 */
void initGpsTask()
{
	xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 8192, NULL, 1, NULL, 0);
	delay(500);
}

/**
 * @brief Command-line interface processing task
 *
 * @details Handles CLI operations including command parsing, execution, and response
 *          generation. Runs on core 1 with 20KB stack size to handle complex CLI
 *          operations and network communications. The task processes commands at
 *          60ms intervals to maintain responsive user interaction.
 *
 * @param param Task parameters (unused in current implementation)
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
 * @brief Initialize CLI processing task
 *
 * @details Creates and starts the CLI task on core 1 with 20KB stack size and priority 1.
 *          Only compiled when CLI functionality is enabled (not DISABLE_CLI).
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 20000, NULL, 1, NULL, 1); }

#endif


