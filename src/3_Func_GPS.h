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
      if (GPS.location.isValid())
        is_gps_fixed = true;
    }
  } while (millis() - start < ms);
}

// **********************************************
//  Función para buscar satélites al iniciar.
// **********************************************
void search_init_sat()
{
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

// *********************************************
//  Función para obtener tileX de los mapas
//  Openstreetmap
//
//  f_lon:    longitud
//  zoom:     zoom
// *********************************************
int lon2tilex(double f_lon, int zoom)
{
   return (int)(floor((f_lon + 180.0) / 360.0 * pow(2.0, zoom))); 
}

// *********************************************
//  Función para obtener tileY de los mapas
//  Openstreetmap
//
//  f_lat:    latitud
//  zoom:     zoom
// *********************************************
int lat2tiley(double f_lat, int zoom)
{
  return (int)(floor((1.0 - log( tan(f_lat * M_PI/180.0) + 1.0 / cos(f_lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, zoom))); 
}

// *********************************************
//  Función para obtener posX de los mapas
//  Openstreetmap
//
//  f_lon:  longitud
//  zoom:   zoom
// *********************************************
int lon2posx(float f_lon, int zoom)
{
   return ((int)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom))*256)) % 256); 
}

// *********************************************
//  Función para obtener posY de los mapas
//  Openstreetmap
//
//  f_lat:  latitud
//  zoom:   zoom
// *********************************************
int lat2posy(float f_lat, int zoom)
{
   return ((int)(((1.0 - log( tan(f_lat * M_PI/180.0) + 1.0 / cos(f_lat * M_PI/180.0)) / M_PI) / 2.0 * (pow(2.0, zoom))*256)) % 256); 
}