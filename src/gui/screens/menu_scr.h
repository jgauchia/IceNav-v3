/**
 * @file menu_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Menu screen
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display options menu screen
 * 
 */
void show_menu_screen()
{
  if (!is_draw)
  {
    tft.writecommand(0x28);
    tft.fillScreen(TFT_WHITE);
    drawBmp("/GFX/BOT_TRAC.BMP", 20, 15, true);
    drawBmp("/GFX/BOT_NAV.BMP", 20, 60, true);
    drawBmp("/GFX/BOT_MAPA.BMP", 20, 105, true);
    drawBmp("/GFX/BOT_BRUJ.BMP", 20, 150, true);
    drawBmp("/GFX/BOT_LOG.BMP", 20, 195, true);
    drawBmp("/GFX/BOT_CFG.BMP", 20, 240, true);
    tft.writecommand(0x29);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    is_menu_screen = true;
    is_map_screen = false;
    is_sat_screen = false;
    is_compass_screen = false;
    is_draw = true;
  }
  show_notify_bar(10, 292);
}
