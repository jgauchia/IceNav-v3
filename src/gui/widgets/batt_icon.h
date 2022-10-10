/**
 * @file show_batt_icon.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Battery icon
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display battery icon at x,y position
 * 
 * @param x -> X position
 * @param y -> Y position
 */
void show_batt_icon(int x, int y)
{
  char s_buf[64];
  tft.setSwapBytes(true);
  if (batt_level > 80 && batt_level <= 100)
    tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_100_icon);
  else if (batt_level <= 80 && batt_level > 60)
    tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_75_icon);
  else if (batt_level <= 60 && batt_level > 40)
    tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_50_icon);
  else if (batt_level <= 40 && batt_level > 20)
    tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_25_icon);
  else if (batt_level <= 20)
    tft.pushImage(x, y, Icon_Notify_Width, Icon_Notify_Height, batt_0_icon);
  tft.setSwapBytes(false);
  sprintf(s_buf, "%3d%%", batt_level);
  tft.drawString(s_buf, x, y + 24, 1);
}