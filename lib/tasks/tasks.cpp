/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.9
 * @date 2024-12
 */

#include "tasks.hpp"

TaskHandle_t LVGLTaskHandler;
xSemaphoreHandle gpsMutex;

/**
 * @brief Read GPS data
 *
 * @param pvParameters
 */
void gpsTask(void *pvParameters)
{
  log_v("GPS Task - running on core %d", xPortGetCoreID());
  log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
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
        getGPSData();
      }

      xSemaphoreGive(gpsMutex);

      vTaskDelay(1); /// portTICK_PERIOD_MS);
    }
  }
}

/**
 * @brief Init GPS task
 *
 */
void initGpsTask()
{
  xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 8192, NULL, 1, NULL, 0);
  delay(500);
}

/**
 * @brief CLI task
 *
 * @param param
 */
#ifndef DISABLE_CLI
void cliTask(void *param) 
{
  log_v("CLI Task - running on core %d", xPortGetCoreID());
  log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
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
 */
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 20000, NULL, 1, NULL, 1); }

#endif


