/*
       @file       4_Func_GFX.h
       @brief      Funciones para tratamiento de gráficos

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// *********************************************
//  Función que dibuja icono de satélite fijado
//
//  x,y:      posición en pantalla
// *********************************************
void show_sat_icon(int x, int y)
{
  /* No Fix */
  if (GPS.location.age() == 4294967295)
    tft.drawBitmap(x, y, sat_bmp, 32, 30, TFT_RED);
  /* Prediction */
  else if (GPS.location.age() > 2000)
    tft.drawBitmap(x, y, sat_bmp, 32, 30, TFT_ORANGE);
  /* Fix */
  else
    tft.drawBitmap(x, y, sat_bmp, 32, 30, TFT_DARKGREEN);
}

// *********************************************
//  Función que dibuja hora
//
//  x,y:      posición en pantalla
//  font:     tipo de letra
// *********************************************
void show_sat_hour(int x, int y, int font)
{
  tft.startWrite();
  tft.setCursor(x, y, font);
  if (hour() < 10) {
    tft.print('0');
  }
  tft.print(hour(), DEC); tft.print(':');
  if (minute() < 10) {
    tft.print('0');
  }
  tft.print(minute(), DEC); tft.print(':');
  if (second() < 10) {
    tft.print('0');
  }
  tft.print(second(), DEC);
  tft.endWrite();
}

// *********************************************
//  Función que dibuja barra notificación
//
//  x,y:      posición en pantalla
// *********************************************
void show_notify_bar(int x, int y)
{
  show_sat_hour(x, y, 4);
  show_sat_icon(x + 120, y - 5);
  show_battery(x + 200, y - 5);
  tft.drawNumber(GPS.satellites.value(), x+155, y, 4);
}

// *********************************************
//  Función que dibuja tracking satélites
// *********************************************
void show_sat_tracking()
{

  Latitude_formatString(5, 5, 2, GPS.location.lat());
  Longitude_formatString(5, 20, 2, GPS.location.lng());
  tft.drawNumber(GPS.satellites.value(), 35, 50 );

  sprintf(s_buf, "%4d m", (int)GPS.altitude.meters());
  tft.drawString(s_buf, 0, 130, 4);
  sprintf(s_buf, "%2.1f", (double)GPS.hdop.hdop());
  tft.drawString(s_buf, 5, 90, 4);

 if (totalGPGSVMessages.isUpdated())
  {
    for (int i = 0; i < 4; ++i)
    {
      int no = atoi(satNumber[i].value());
      if (no >= 1 && no <= MAX_SATELLITES)
      {
        sat_tracker[no - 1].elevation = atoi(elevation[i].value());
        sat_tracker[no - 1].azimuth = atoi(azimuth[i].value());
        sat_tracker[no - 1].snr = atoi(snr[i].value());
        sat_tracker[no - 1].active = true;
      }
    }

    int totalMessages = atoi(totalGPGSVMessages.value());
    int currentMessage = atoi(messageNumber.value());
    if (totalMessages == currentMessage)
    {
      for (int i = 0; i < MAX_SATELLITES; ++i)
      {
        if (sat_tracker[i].pos_x != 0 && sat_tracker[i].pos_y != 0 )
        {
          sat_sprite.fillCircle(4, 4, 4, TFT_WHITE);
          sat_sprite.pushSprite(sat_tracker[i].pos_x, sat_tracker[i].pos_y);
          tft.startWrite();
          tft.setCursor(sat_tracker[i].pos_x, sat_tracker[i].pos_y + 5, 1);
          tft.print("  ");
          tft.endWrite();
        }
      }
      tft.startWrite();
      tft.drawCircle(165, 80, 60, TFT_BLACK);
      tft.drawCircle(165, 80, 30, TFT_BLACK);
      tft.drawCircle(165, 80, 1, TFT_BLACK);
      tft.drawString("N", 162, 12, 2);
      tft.drawString("S", 162, 132, 2);
      tft.drawString("O", 102, 72, 2);
      tft.drawString("E", 222, 72, 2);
      tft.endWrite();

      int active_sat = 0;
      for (int i = 0; i < MAX_SATELLITES; ++i)
      {
        if ( i < 12)
          tft.pushRect((i *  20), 159, 25, 80, snr_bkg);
        else
          tft.pushRect(((i - 12) * 20), 240, 25, 80, snr_bkg);
        if (sat_tracker[i].active)
        {
          if ( active_sat < 12 )
          {
            tft.setCursor((active_sat * 20) + 8, 229, 1);
            tft.fillRect((active_sat * 20) + 5, 224 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_DARKCYAN);
          }
          else
          {
            tft.setCursor(((active_sat - 12 ) * 20) + 8, 310, 1);
            tft.fillRect(((active_sat - 12 ) * 20) + 5, 305 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_DARKCYAN);
          }
          tft.print(i + 1);
          active_sat++;
          int H = (60 * cos(DEGtoRAD(sat_tracker[i].elevation)));
          int sat_pos_x = 165 + (H * sin(DEGtoRAD(sat_tracker[i].azimuth)));
          int sat_pos_y = 80 - (H * cos(DEGtoRAD(sat_tracker[i].azimuth)));
          sat_tracker[i].pos_x = sat_pos_x;
          sat_tracker[i].pos_y = sat_pos_y;
          sat_sprite.fillCircle(4, 4, 4, TFT_GREEN);
          sat_sprite.pushSprite(sat_pos_x, sat_pos_y);
          tft.setCursor(sat_pos_x, sat_pos_y + 5, 1);
          tft.print(i + 1);
        }
      }
    }
  }
}
