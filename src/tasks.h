/**
 * @file tasks.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.1.4
 * @date 2023-05-23
 */

/**
 * @brief Task 1 - Read GPS data
 *
 * @param pvParameters
 */
void Read_GPS(void *pvParameters)
{
  debug->print(PSTR("Task1 - Read GPS - running on core "));
  debug->println(xPortGetCoreID());
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
#endif
    }
    vTaskDelay(1);
  }
}

/**
 * @brief Init Core tasks
 *
 */
void init_tasks()
{
  xTaskCreatePinnedToCore(Read_GPS, PSTR("Read GPS"), 1000, NULL, 1, NULL, 1);
  delay(500);
}