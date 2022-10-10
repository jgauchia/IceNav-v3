/**
 * @file hour.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Display time
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display time at x,y postion and font type
 * 
 * @param x -> X Position
 * @param y -> Y POsition
 * @param font -> Font type
 */
void show_sat_hour(int x, int y, int font)
{
  tft.startWrite();
  tft.setTextFont(font);
  tft.setCursor(x, y, font);
  if (hour() < 10)
  {
    tft.print('0');
  }
  tft.print(hour(), DEC);
  tft.print(':');
  if (minute() < 10)
  {
    tft.print('0');
  }
  tft.print(minute(), DEC);
  tft.print(':');
  if (second() < 10)
  {
    tft.print('0');
  }
  tft.print(second(), DEC);
  tft.endWrite();
}