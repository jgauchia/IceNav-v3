/**
 * @file tasks.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Task 1 - Read GPS data
 *
 * @param pvParameters
 */
void Read_GPS(void *pvParameters)
{
  log_v("Task1 - Read GPS - running on core %d", xPortGetCoreID());
  log_v("Stack size: %d",uxTaskGetStackHighWaterMark(NULL));
  for (;;)
  {
    while (gps->available() > 0)
    {
#ifdef OUTPUT_NMEA
      {
        debug->write(gps->read());
      }
#else
      GPS.encode(gps->read());
      vTaskDelay(10);
#endif
    }
    
  }
}

/**
 * @brief Task2 - LVGL Task
 * 
 * @param pvParameters 
 */
void LVGL_Task(void *pvParameters)
{
  log_v("Task2 - LVGL Task - running on core %d", xPortGetCoreID());
  for (;;)
  {
    vTaskDelay(10); 
   // lv_tick_inc(5);
  }
}

/**
 * @brief Init Core tasks
 *
 */
void init_tasks()
{
  xTaskCreatePinnedToCore(Read_GPS, PSTR("Read GPS"), 20000, NULL, 3, NULL, 1);
  delay(500);
  //xTaskCreatePinnedToCore(LVGL_Task, PSTR("LVGL Task"), 20000, NULL, 1, NULL, 1);
  //delay(500);
}