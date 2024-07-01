/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "tasks.hpp"

time_t local, utc = 0;

TaskHandle_t LVGLTaskHandler;

/**
 * @brief LVGL Task
 *
 * @param pvParameters
 */
void lvglTask(void *pvParameters)
{
  log_v("LVGL Task - running on core %d", xPortGetCoreID());
  log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
  while (1)
  {
    lv_timer_handler();
    vTaskDelay(5);
  }
}

/**
 * @brief Init LVGL Task
 * 
 */
void initLvglTask()
{
  xTaskCreatePinnedToCore(lvglTask, PSTR("LVGL Task"), 20000, NULL, 1, &LVGLTaskHandler, 0);
  delay(500);
}

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
    while (gps->available() > 0)
    {
      #ifdef OUTPUT_NMEA
      {
        Serial.write(gps->read());
      }
      #else
      GPS.encode(gps->read());
      #endif
    }
    if (GPS.time.isValid() && !isTimeFixed)
    {
      setTime(GPS.time.hour(),
              GPS.time.minute(),
              GPS.time.second(),
              GPS.date.day(),
              GPS.date.month(),
              GPS.date.year());
      utc = now();
      local = CE.toLocal(utc);
      setTime(local);
      isTimeFixed = true;
    }
    // if (!GPS.time.isValid() && isTimeFixed)
    //     isTimeFixed = false;

        vTaskDelay(10 / portTICK_PERIOD_MS);

  }
}

/**
 * @brief Init GPS task
 *
 */
void initGpsTask()
{
  xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 8192, NULL, 1, NULL, 1);
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
void initCLITask() { xTaskCreatePinnedToCore(cliTask, "cliTask ", 4000, NULL, 1, NULL, 1); }

#endif

#ifdef ENABLE_COMPASS
/**
 * @brief Read Compass data task
 *
 * @param pvParameters
 */
void compassTask(void *pvParameters)
{
  log_v("Compass Task - running on core %d", xPortGetCoreID());
  log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
  while (1)
  {
    heading = getHeading();
    vTaskDelay(200);
  }
}

/**
 * @brief Init Compass data task
 *
 */
void initCompassTask()
{
  xTaskCreatePinnedToCore(compassTask, PSTR("Compass Task"), 8192, NULL,tskIDLE_PRIORITY , NULL, 1);
  delay(500);
}
#endif
