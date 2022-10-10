/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass indicator
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Create a compass sprite object
 * 
 */
void create_compass_sprite()
{
  compass_sprite.deleteSprite();
  compass_sprite.setColorDepth(8);
  compass_sprite.createSprite(205, 205);
  compass_sprite.fillScreen(TFT_BLACK);
  compass_sprite.fillCircle(102, 102, 105, TFT_WHITE);
  compass_sprite.fillCircle(102, 102, 98, TFT_DARKCYAN);
  compass_sprite.fillCircle(102, 102, 90, TFT_WHITE);
  compass_sprite.fillCircle(102, 102, 80, TFT_BLACK);
  compass_sprite.setTextColor(TFT_DARKCYAN, TFT_WHITE);
  compass_sprite.drawString("N", 95, 0, 4);
  compass_sprite.drawString("S", 95, 185, 4);
  compass_sprite.drawString("W", 0, 95, 4);
  compass_sprite.drawString("E", 185, 95, 4);
  tft.setPivot(118, 207);
}

/**
 * @brief Compass indicator
 * 
 */
void show_Compass()
{
#ifdef ENABLE_PCF8574
  if (key_pressed == LBUT && is_show_degree)
    is_show_degree = false;
  else if (key_pressed == LBUT && !is_show_degree)
    is_show_degree = true;
#endif

#ifdef ENABLE_COMPASS
  char s_buf[64];
  int rumbo = Read_Mag_data();
  compass_sprite.pushRotated(360 - rumbo, TFT_BLACK);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillRect(55, 207, 130, 40, TFT_WHITE);
  if (!is_show_degree)
  {
    int altura = (int)GPS.altitude.meters();
    if (altura < 10)
      sprintf(s_buf, "%s%1d", "      ", altura);
    else if (altura < 100)
      sprintf(s_buf, "%s%2d", "    ", altura);
    else if (altura < 1000)
      sprintf(s_buf, "%s%3d", "  ", altura);
    else
      sprintf(s_buf, "%4d", altura);
    tft.drawString(s_buf, 55, 207, 6);
    tft.drawString("m", 165, 225, 4);
  }
  else
  {
    if (rumbo < 10)
      sprintf(s_buf, "%s%1d", "    ", rumbo);
    else if (rumbo < 100)
      sprintf(s_buf, "%s%2d", "  ", rumbo);
    else
      sprintf(s_buf, "%3d", rumbo);
    tft.drawString(s_buf, 75, 207, 6);
    tft.setTextFont(4);
    tft.setCursor(165, 207);
    tft.print("`");
  }
#endif
}