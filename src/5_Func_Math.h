
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
