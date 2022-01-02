/*
       @file       5_Func_Math.h
       @brief      Funciones para cálculos varios

       @author     Jordi Gauchia

       @date       08/12/2021
*/


// *********************************************
//  Función para hacer un map con floats
//
//  x:        valor
//  in_min:   valor minimo
//  in_max:   valor máximo
//  out_min:  valor salida mínimo
//  out_max:  valor salida máximo.
// *********************************************
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// *********************************************
//  Función conversión radianes a grados
//
//  rad:      Radianes
// *********************************************
double RADtoDEG(double rad)
{
  return rad * 180 / PI;
}

// *********************************************
//  Función conversión grados a radianes
//
//  deg:      Gradpos
// *********************************************
double DEGtoRAD(double deg)
{
  return deg * PI / 180;
}

// *********************************************
//  Función conversión latitud a GGºMM'SS" a str
//
//  x,y:      posición
//  font:     tipo letra
//  lat:      latitud
// *********************************************
void Latitude_formatString(int x, int y, int font,  double lat)
{
  char N_S = 'N';
  double absLatitude = lat;
  uint16_t deg;
  uint8_t min;
  if (lat < 0)
  {
    N_S = 'S';
    absLatitude = fabs(lat);
  }
  deg = (uint16_t) absLatitude;
  absLatitude = (absLatitude - deg) * 60;
  min = (uint8_t) absLatitude;
  absLatitude = (absLatitude - min) * 60;
  tft.setTextFont(font);
  tft.setCursor(x, y, font);
  sprintf(s_buf, "%03d ", deg);
  tft.print(s_buf);
  tft.print("`");
  sprintf(s_buf, "%02d\' %.2f\" %c", min, absLatitude, N_S);
  tft.print(s_buf);
}

// *********************************************
//  Función conversión longitud a GGºMM'SS" a str
//
//  x,y:      posición
//  font:     tipo letra
//  lon:      longitud
// *********************************************
void Longitude_formatString(int x, int y, int font,  double lon)
{
  char E_W = 'E';
  double absLongitude = lon;
  uint16_t deg;
  uint8_t min;
  if (lon < 0)
  {
    E_W = 'W';
    absLongitude = fabs(lon);
  }
  deg = (uint16_t) absLongitude;
  absLongitude = (absLongitude - deg) * 60;
  min = (uint8_t) absLongitude;
  absLongitude = (absLongitude - min) * 60;
  tft.setTextFont(font);
  tft.setCursor(x, y, font);
  sprintf(s_buf, "%03d ", deg);
  tft.print(s_buf);
  tft.print("`");
  sprintf(s_buf, "%02d\' %.2f\" %c", min, absLongitude, E_W);
  tft.print(s_buf);
}
