/**
 * @file tasks.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Core Tasks functions
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Task 1 - Read GPS data
 *
 * @param pvParameters
 */
void Read_GPS(void *pvParameters)
{
  debug->print("Task1 - Read GPS - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {
    if (gps->available())
    {
      GPS.encode(gps->read());

#ifdef OUTPUT_NMEA
      {
        debug->write(gps->read());
      }
#endif
    }
    delay(1);
  }
}

/**
 * @brief Task 2 - Main program
 *
 * @param pvParameters
 */
void Main_prog(void *pvParameters)
{
  debug->print("Task2 - Main Program - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {
    currentScreen = lv_scr_act();
    heading = read_compass();
    // if (BATTtime.update())
    //   batt_level = Read_Battery();

    delay(1);
  }
}

/**
 * @brief Init Core tasks
 *
 */
void init_tasks()
{
  xTaskCreatePinnedToCore(Read_GPS, "Read GPS", 16384, NULL, 4, NULL, 0);
  delay(500);
  xTaskCreatePinnedToCore(Main_prog, "Main Program", 16384, NULL, 1, NULL, 1);
  delay(500);
}