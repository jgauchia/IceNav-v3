/**
 * @file sat_icon.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite icon
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display satellite icon at x,y position
 *
 * @param x -> X position
 * @param y -> Y position
 */
void show_sat_icon(int x, int y)
{
  tft.setSwapBytes(true);
  tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, satellite_icon);
  tft.setSwapBytes(false);
  tft.drawNumber(GPS.satellites.value(), x + 10, y + 20, 2);
}