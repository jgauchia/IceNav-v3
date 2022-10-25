/**
 * @file map_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief  Display map screen, GPS Ubication on map, hour, satellites, battery
 * 
 */
void show_map_screen()
{
  char s_buf[64];
  if (!is_draw)
  {
    tft.fillScreen(TFT_BLACK);
    tft.drawLine(0, 40, 240, 40, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("ZOOM:", 5, 45, 2);
    is_map_screen = true;
    is_menu_screen = false;
    is_sat_screen = false;
    is_compass_screen = false;
    is_draw = true;
  }
  show_notify_bar(10, 10);
  if (is_gps_fixed)
  {
    show_map(0, 64, GPS.location.lng(), GPS.location.lat());
    sprintf(s_buf, "%2d", zoom);
    tft.drawString(s_buf, 45, 45, 2);
  }
}

