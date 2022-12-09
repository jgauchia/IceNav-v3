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
    int totalMessages = atoi(totalGPGSVMessages.value());
    int currentMessage = atoi(messageNumber.value());
    if (totalMessages == currentMessage)
    {
      for (int i = 0; i < MAX_SATELLITES; ++i)
      {
        if (sat_tracker[i].pos_x != 0 && sat_tracker[i].pos_y != 0)
        {
          sat_sprite.fillCircle(2, 2, 2, TFT_BLACK);
          sat_sprite.pushSprite(sat_tracker[i].pos_x, sat_tracker[i].pos_y,TFT_TRANSPARENT);
          tft.startWrite();
          tft.setCursor(sat_tracker[i].pos_x, sat_tracker[i].pos_y + 5, 1);
          tft.print("  ");
          tft.endWrite();
        }
      }
      tft.startWrite();
      tft.drawCircle(165, 80, 60, TFT_WHITE);
      tft.drawCircle(165, 80, 30, TFT_WHITE);
      tft.drawCircle(165, 80, 1, TFT_WHITE);
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
            tft.fillRect((active_sat * 20) + 5, 224 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_RED);
          }
          else
          {
            tft.setCursor(((active_sat - 12) * 20) + 8, 310, 1);
            tft.fillRect(((active_sat - 12) * 20) + 5, 305 - (sat_tracker[i].snr), 15, (sat_tracker[i].snr), TFT_RED);
          }
          tft.print(i + 1);
          active_sat++;
          int H = (60 * cos(DEGtoRAD(sat_tracker[i].elevation)));
          int sat_pos_x = 165 + (H * sin(DEGtoRAD(sat_tracker[i].azimuth)));
          int sat_pos_y = 80 - (H * cos(DEGtoRAD(sat_tracker[i].azimuth)));
          sat_tracker[i].pos_x = sat_pos_x;
          sat_tracker[i].pos_y = sat_pos_y;
          sat_sprite.fillCircle(2, 2, 2, TFT_GREEN);
          sat_sprite.pushSprite(sat_pos_x, sat_pos_y, TFT_TRANSPARENT);
          tft.setCursor(sat_pos_x, sat_pos_y + 5, 1);
          tft.print(i + 1);
        }
      }
    }
  }
}

