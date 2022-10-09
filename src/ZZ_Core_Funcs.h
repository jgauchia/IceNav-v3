/*
       @file       ZZ_Core_Funcs.h
       @brief      Funciones necesarias el tratamiento de tareas y cores del ESP32

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Función para inicializar tareas del ESP32
// **********************************************
void init_tasks()
{
  xTaskCreatePinnedToCore(Read_GPS, "Read GPS", 16384, NULL, 4, NULL, 0);
  delay(500);
  xTaskCreatePinnedToCore(Main_prog, "Main Program", 16384, NULL, 1, NULL, 1);
  delay(500);
}

// **********************************************
//  Función para tarea de lectura GPS
// **********************************************
void Read_GPS(void *pvParameters)
{
  debug->print("Task1 - Read GPS - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {
    if (gps->available() > 1)
    {
      GPS.encode(gps->read());
      if (GPS.location.isValid())
        is_gps_fixed = true;

#ifdef OUTPUT_NMEA
      if (gps->available())
      {
        debug->println(GPS.location.lat());
      }
#endif
    }
    delay(1);
  }
}

// **********************************************
//  Función para tarea del navegador
// **********************************************
void Main_prog(void *pvParameters)
{
  debug->print("Task2 - Main Program - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {

#ifdef ENABLE_PCF8574
    key_pressed = Read_Keys();
    debug->println(key_pressed);
    if (KEYStime.update())
      Check_keys(key_pressed);
#endif
    if (BATTtime.update())
      batt_level = Read_Battery();

    if (is_menu_screen)
    {
      show_menu_screen();
    }
    else if (!is_menu_screen)
    {
      if (!is_map_screen)
        zoom_old = tilex = tiley = 0;
      MainScreen[sel_MainScreen]();
    }
    delay(1);
  }
}
