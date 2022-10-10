/**
 * @file sat_track_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite tracking screen
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Get the sat tracking info (elevation, azimunt, snr, active,...)
 * 
 */
void get_sat_tracking()
{
  char s_buf[64];
  Latitude_formatString(5, 5, 2, GPS.location.lat());
  Longitude_formatString(5, 20, 2, GPS.location.lng());
  tft.drawNumber(GPS.satellites.value(), 35, 50);

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
        if (sat_tracker[i].pos_x != 0 && sat_tracker[i].pos_y != 0)
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
        if (i < 12)
          tft.pushRect((i * 20), 159, 25, 80, snr_bkg);
        else
          tft.pushRect(((i - 12) * 20), 240, 25, 80, snr_bkg);
        if (sat_tracker[i].active)
        {
          if (active_sat < 12)
          {
            tft.setCursor((active_sat * 20) + 8, 229, 1);
            tft.fillRect((active_sat * 20) + 5, 224 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_DARKCYAN);
          }
          else
          {
            tft.setCursor(((active_sat - 12) * 20) + 8, 310, 1);
            tft.fillRect(((active_sat - 12) * 20) + 5, 305 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_DARKCYAN);
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

/**
 * @brief Display satellite track screen, GPS Position, number satellites, location
 * 
 */
void show_sat_track_screen()
{
  if (!is_draw)
  {
    tft.startWrite();
    sat_sprite.deleteSprite();
    sat_sprite.createSprite(8, 8);
    sat_sprite.fillScreen(TFT_WHITE);
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.fillRect(0, 159, 240, 2, TFT_BLACK);
    tft.fillRect(0, 240, 240, 2, TFT_BLACK);
    for (int i = 0; i < 7; i++)
    {
      tft.drawLine(0, 224 - (i * 10), 240, 224 - (i * 10), TFT_LIGHTGREY);
      tft.drawLine(0, 305 - (i * 10), 240, 305 - (i * 10), TFT_LIGHTGREY);
    }
    tft.readRect(0, 159, 25, 80, snr_bkg);
    tft.drawCircle(165, 80, 60, TFT_BLACK);
    tft.drawCircle(165, 80, 30, TFT_BLACK);
    tft.drawCircle(165, 80, 1, TFT_BLACK);
    tft.drawString("N", 162, 12, 2);
    tft.drawString("S", 162, 132, 2);
    tft.drawString("O", 102, 72, 2);
    tft.drawString("E", 222, 72, 2);
    tft.drawString("Altura", 5, 115, 2);
    tft.drawString("HDOP", 5, 75, 2);
    tft.drawString("Sat:", 5, 50, 2);
    tft.endWrite();
    is_sat_screen = true;
    is_menu_screen = false;
    is_map_screen = false;
    is_compass_screen = false;
    is_draw = true;
  }
  get_sat_tracking();
}