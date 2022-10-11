/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Main screen
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display main screen, GPS Position, hour, satellites, battery and compass
 *
 */
void show_main_screen()
{
  if (!is_draw)
  {
    tft.writecommand(0x28);
    drawBmp("/GFX/POSICION.bmp", 5, 44, true);
    tft.writecommand(0x29);
    tft.fillScreen(TFT_BLACK);
    tft.drawLine(0, 40, 240, 40, TFT_WHITE);
    tft.setSwapBytes(true);
#ifdef ENABLE_COMPASS
    tft.pushImage(95, 135, 50, 58, compass_arrow);
    create_compass_sprite();
#endif
    tft.setSwapBytes(false);
    is_compass_screen = true;
    is_map_screen = false;
    is_menu_screen = false;
    is_sat_screen = false;
    is_draw = true;
  }
#ifdef ENABLE_COMPASS
  show_Compass();
#endif

  tft.startWrite();
  Latitude_formatString(50, 45, 2, GPS.location.lat());
  Longitude_formatString(50, 60, 2, GPS.location.lng());
  show_notify_bar(10, 10);
  tft.endWrite();
}