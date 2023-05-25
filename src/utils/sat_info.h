/**
 * @file sat_info.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellites info screen functions
 * @version 0.1.4
 * @date 2023-05-23
 */

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

void fill_sat_in_view(GSV &gsv)
{

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
}