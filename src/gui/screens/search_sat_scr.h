/**
 * @file search_sat_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS satellite search screen
 * @version 0.1
 * @date 2022-10-09
 */

/**
 * @brief GPS satellite search screen
 * 
 */
void search_sat_scr()
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