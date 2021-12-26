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
  xTaskCreatePinnedToCore(Read_GPS, "Read GPS"    ,  8192, NULL, 1, &Task1, 0);
  delay(500);
  xTaskCreatePinnedToCore(Main_prog,"Main Program", 16384, NULL, 1, &Task2, 1);
  delay(500);
}

// **********************************************
//  Función para tarea de lectura GPS
// **********************************************
void Read_GPS( void * pvParameters ) {
  debug->print("Task1 - Read GPS - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {
    read_NMEA(GPS_UPDATE_TIME);
    delay(1);
  }
}

// **********************************************
//  Función para tarea del navegador
// **********************************************
void Main_prog( void * pvParameters ) {
  debug->print("Task2 - Main Program - running on core ");
  debug->println(xPortGetCoreID());
  for (;;)
  {
    batt_level = Read_Battery();
    key_pressed = Read_Keys();
    Check_keys(key_pressed);
    if (is_menu_screen)
    {
      show_menu_screen();
    }
    else
    {
      show_main_screen();
      //show_sat_track_screen();
    }
    delay(1);
  }
}
