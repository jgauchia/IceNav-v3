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
  compass_sprite.fillSprite(TFT_TRANSPARENT);
  for (int count = 1; count <= 15; count++)
  {
    compass_sprite.drawCircle(102, 102, 94 + count, TFT_BLACK);
    compass_sprite.drawCircle(102, 102, 92 - count, TFT_BLACK);
  }
  compass_sprite.drawCircle(102, 102, 92, TFT_WHITE);
  compass_sprite.drawCircle(102, 102, 93, TFT_WHITE);
  compass_sprite.drawCircle(102, 102, 94, TFT_WHITE);

  compass_sprite.setTextColor(TFT_WHITE, TFT_BLACK);
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
  int altura = (int)GPS.altitude.meters();
  compass_sprite.pushRotated(360 - rumbo, TFT_TRANSPARENT);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("8888", 6));
  if (!is_show_degree)
  {
    sprintf(s_buf, "%4d", altura);
    tft.drawRightString(s_buf, 55 + tft.textWidth("8888", 6), 207, 6);
    tft.setTextPadding(0);
    tft.drawString("m", 165, 225, 4);
    tft.setTextFont(4);
    tft.setCursor(165, 205);
    tft.print("   ");
  }
  else
  {
    sprintf(s_buf, "%3d", rumbo);
    tft.drawRightString(s_buf, 55 + tft.textWidth("8888", 6), 207, 6);
    tft.setTextPadding(0);
    tft.setTextFont(4);
    tft.setCursor(165, 207);
    tft.print("`");
    tft.setCursor(165, 225);
    tft.print("    ");
  }
#endif
}