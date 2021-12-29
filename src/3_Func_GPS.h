/*
       @file       3_Func_GPS.h
       @brief      Funciones necesarias para tratar GPS

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Función para leer y pasear sentencias NMEA.
//
//  ms:  frecuencia actualización gps
// **********************************************
void read_NMEA(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (gps->available())
    {
      GPS.encode(gps->read());
    }
  } while (millis() - start < ms);
}

// **********************************************
//  Función para buscar satélites al iniciar.
// **********************************************
void search_init_sat()
{
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Buscando Satelites", 10, 100, 4);
  millis_actual = millis();
  while (!GPS.location.isValid())
  {
    for (int i = 0; i < 11; i++ )
    {
      tft.drawString("o ", 12 + (20 * i), 150, 4);
      read_NMEA(1000); 
      if (GPS.location.isValid())
      {
        is_gps_fixed = true;
        setTime(GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.date.day(), GPS.date.month(), GPS.date.year());
        delay(50);
        adjustTime(TIME_OFFSET * SECS_PER_HOUR);
        delay(500);
        break;
      }
    }
    tft.fillRect(12, 150, 320, 180, TFT_BLACK);
  }
}
