/**
 * @file sat_info.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellites info screen functions
 * @version 0.1.4
 * @date 2023-05-23
 */

#include <lvgl.h>

/**
 * @brief Structure to store satellite position in constelation map
 *
 */
struct SatPos
{
  uint16_t x;
  uint16_t y;
};

/**
 * @brief Satellite position X,Y
 *
 */
SatPos sat_pos;

/**
 * @brief Sprite for snr GPS Satellite Labels
 *
 */
TFT_eSprite spr_SNR1 = TFT_eSprite(&tft);
TFT_eSprite spr_SNR2 = TFT_eSprite(&tft);

/**
 * @brief Sprite for satellite position in map
 *
 */
TFT_eSprite spr_Sat = TFT_eSprite(&tft);

/**
 * @brief Double Buffering Sprites for Satellite Constellation
 *
 */
TFT_eSprite constel_spr = TFT_eSprite(&tft);
TFT_eSprite constel_spr_bkg = TFT_eSprite(&tft);

/**
 * @brief Satellite Signal Graphics Bars Definitions
 *
 */
static lv_obj_t *satbar_1;
static lv_obj_t *satbar_2;
static lv_chart_series_t *satbar_ser1;
static lv_chart_series_t *satbar_ser2;

/**
 * @brief Get the Satellite position for constelation map
 *
 * @param elev -> elevation
 * @param azim -> Azimut
 * @return SatPos -> Satellite position
 */
SatPos get_sat_pos(uint8_t elev, uint16_t azim)
{
  SatPos pos;
  int H = (60 * cos(DEGtoRAD(elev)));
  pos.x = 100 + (H * sin(DEGtoRAD(azim)));
  pos.y = 75 - (H * cos(DEGtoRAD(azim)));
  return pos;
}

/**
 * @brief Create Constellation sprite
 *
 * @param spr -> Sprite
 */
void create_const_spr(TFT_eSprite &spr)
{
  spr.deleteSprite();
  spr.createSprite(200, 150);
  spr.fillScreen(LVGL_BKG);
  spr.drawCircle(100, 75, 60, TFT_WHITE);
  spr.drawCircle(100, 75, 30, TFT_WHITE);
  spr.drawCircle(100, 75, 1, TFT_WHITE);
  spr.setTextFont(2);
  spr.setTextColor(TFT_WHITE, LVGL_BKG);
  spr.drawString("N", 97, 7);
  spr.drawString("S", 97, 127);
  spr.drawString("W", 37, 67);
  spr.drawString("E", 157, 67);
  spr.setTextFont(1);
}

/**
 * @brief Create satellite sprite
 *
 * @param spr -> Sprite
 */
void create_sat_spr(TFT_eSprite &spr)
{
  spr.deleteSprite();
  spr.createSprite(8, 8);
  spr.setColorDepth(16);
  spr.fillScreen(LVGL_BKG);
}

/**
 * @brief Create SNR text sprite
 *
 * @param spr -> Sprite
 */
void create_snr_spr(TFT_eSprite &spr)
{
  spr.deleteSprite();
  spr.createSprite(TFT_WIDTH, 10);
  spr.fillScreen(LVGL_BKG);
  spr.setTextColor(TFT_WHITE, LVGL_BKG);
}

/**
 * @brief Draw SNR bar and satellite number
 *
 * @param bar -> Bar Control
 * @param bar_ser -> Bar Control Serie
 * @param id -> Active Sat
 * @param sat_num -> Sat ID
 * @param snr -> Sat SNR
 * @param spr -> Sat number sprite
 */
void draw_snr_bar(lv_obj_t *bar, lv_chart_series_t *bar_ser, uint8_t id, uint8_t sat_num, uint8_t snr, TFT_eSprite &spr)
{
  lv_point_t p;
  bar_ser->y_points[id] = snr;
  lv_chart_get_point_pos_by_id(bar, bar_ser, id, &p);
  spr.setCursor(p.x - 2, 0);
  spr.print(sat_num);
}

/**
 * @brief Clear Satellite in View found
 * 
 */
void clear_sat_in_view()
{
  for (int clear = 0; clear < MAX_SATELLITES; clear++)
  {
    sat_tracker[clear].sat_num = 0;
    sat_tracker[clear].elev = 0;
    sat_tracker[clear].azim = 0;
    sat_tracker[clear].snr = 0;
    sat_tracker[clear].active = false;
  }
}

/**
 * @brief Display satellite in view info
 *
 * @param gsv -> GSV NMEA sentence
 * @param color -> Satellite color in constellation
 */
void fill_sat_in_view(GSV &gsv, int color)
{
  if (gsv.totalMsg.isUpdated())
  {
    lv_chart_refresh(satbar_1);
    lv_chart_refresh(satbar_2);

    for (int i = 0; i < 4; ++i)
    {
      int no = atoi(gsv.satNum[i].value());
      if (no >= 1 && no <= MAX_SATELLITES)
      {
        sat_tracker[no - 1].sat_num = atoi(gsv.satNum[i].value());
        sat_tracker[no - 1].elev = atoi(gsv.elev[i].value());
        sat_tracker[no - 1].azim = atoi(gsv.azim[i].value());
        sat_tracker[no - 1].snr = atoi(gsv.snr[i].value());
        sat_tracker[no - 1].active = true;
      }
    }

    uint8_t totalMessages = atoi(gsv.totalMsg.value());
    uint8_t currentMessage = atoi(gsv.msgNum.value());

    if (totalMessages == currentMessage)
    {
      create_snr_spr(spr_SNR1);
      create_snr_spr(spr_SNR2);

      for (int i = 0; i < (MAX_SATELLLITES_IN_VIEW / 2); i++)
      {
        satbar_ser1->y_points[i] = LV_CHART_POINT_NONE;
        satbar_ser2->y_points[i] = LV_CHART_POINT_NONE;
      }

      uint8_t active_sat = 0;
      for (int i = 0; i < MAX_SATELLITES; ++i)
      {
        if (sat_tracker[i].active) // && sat_tracker[i].snr > 0)
        {
          if (active_sat < (MAX_SATELLLITES_IN_VIEW / 2))
            draw_snr_bar(satbar_1, satbar_ser1, active_sat, sat_tracker[i].sat_num, sat_tracker[i].snr, spr_SNR1);
          else
            draw_snr_bar(satbar_2, satbar_ser2, (active_sat - (MAX_SATELLLITES_IN_VIEW / 2)), sat_tracker[i].sat_num, sat_tracker[i].snr, spr_SNR2);

          active_sat++;

          sat_pos = get_sat_pos(sat_tracker[i].elev, sat_tracker[i].azim);

          spr_Sat.fillCircle(4, 4, 2, color);
          spr_Sat.pushSprite(&constel_spr, sat_pos.x, sat_pos.y, TFT_TRANSPARENT);
          constel_spr.setCursor(sat_pos.x, sat_pos.y + 8);
          constel_spr.print(i + 1);

          if (sat_tracker[i].pos_x != sat_pos.x || sat_tracker[i].pos_y != sat_pos.y)
          {
            constel_spr_bkg.pushSprite(120, 30);
          }

          sat_tracker[i].pos_x = sat_pos.x;
          sat_tracker[i].pos_y = sat_pos.y;
        }
      }
      constel_spr.pushSprite(120, 30);
    }

    lv_chart_refresh(satbar_1);
    spr_SNR1.pushSprite(0, 260);
    lv_chart_refresh(satbar_2);
    spr_SNR2.pushSprite(0, 345);
  }
}
