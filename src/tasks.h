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
      if (GPS.location.isValid())
        is_gps_fixed = true;

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
    //currentScreen = lv_scr_act();

    //key_pressed = Read_Keys();
    //if (KEYStime.update())
     // Check_keys(key_pressed);

    if (BATTtime.update())
      batt_level = Read_Battery();

    //   if (is_gps_fixed)
    //   {
    //     if (is_menu_screen)
    //     {
    //       show_menu_screen();
    //     }
    //     else if (!is_menu_screen)
    //     {
    //       if (!is_map_screen)
    //         zoom_old = tilex = tiley = 0;
    //       MainScreen[sel_MainScreen]();
    //     }
    //     delay(1);
    //   }
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