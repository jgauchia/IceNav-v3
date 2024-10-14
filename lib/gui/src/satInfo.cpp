/**
 * @file satInfo.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Satellites info screen functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "satInfo.hpp"
#include "globalGuiDef.h"

// GSV gnssInfoSV; // GPS Satellites in view
// GSV GL_GSV;  // GLONASS Satellites in view
// GSV BD_GSV;  // BEIDOU Satellites in view

SatPos satPos; // Satellite position X,Y in constellation map

TFT_eSprite spriteSNR1 = TFT_eSprite(&tft);       // Sprite for snr GPS Satellite Labels
TFT_eSprite spriteSNR2 = TFT_eSprite(&tft);       // Sprite for snr GPS Satellite Labels
TFT_eSprite spriteSat = TFT_eSprite(&tft);        // Sprite for satellite position in map
TFT_eSprite constelSprite = TFT_eSprite(&tft);    // Sprite for Satellite Constellation

lv_obj_t *satelliteBar1;               // Satellite Signal Graphics Bars
lv_obj_t *satelliteBar2;               // Satellite Signal Graphics Bars
lv_chart_series_t *satelliteBarSerie1; // Satellite Signal Graphics Bars
lv_chart_series_t *satelliteBarSerie2; // Satellite Signal Graphics Bars

/**
 * @brief Get the Satellite position for constellation map
 *
 * @param elev -> elevation
 * @param azim -> Azimuth
 * @return SatPos -> Satellite position
 */
SatPos getSatPos(uint8_t elev, uint16_t azim)
{
  SatPos pos;
  int H = (60 * cos(DEG2RAD(elev)));
  pos.x = 75  + (H * sin(DEG2RAD(azim)));
  pos.y = 75 - (H * cos(DEG2RAD(azim)));
  return pos;
}

/**
 * @brief Delete sat info screen sprites and release PSRAM
 *
 */
void deleteSatInfoSprites()
{
  spriteSNR1.deleteSprite();
  spriteSNR2.deleteSprite();
  spriteSat.deleteSprite();
  constelSprite.deleteSprite();
}

/**
 * @brief Create Constellation sprite
 *
 * @param spr -> Sprite
 */
void createConstelSprite(TFT_eSprite &spr)
{
  spr.createSprite(150 * scale, 150 * scale);
  spr.fillScreen(TFT_BLACK);
  spr.drawCircle(75 * scale, 75 * scale, 60 * scale, TFT_WHITE);
  spr.drawCircle(75 * scale, 75 * scale, 30 * scale, TFT_WHITE);
  spr.drawCircle(75 * scale, 75 * scale, 1, TFT_WHITE);
  #ifdef LARGE_SCREEN
    spr.setTextFont(2);
  #else
    spr.setTextFont(1);
  #endif
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("N", 72 * scale, 7);
  spr.drawString("S", 72 * scale, 127 * scale);
  spr.drawString("W", 12 * scale, 67 * scale);
  spr.drawString("E", 132 * scale, 67 * scale);
  spr.setTextFont(1);
}

/**
 * @brief Create satellite sprite
 *
 * @param spr -> Sprite
 */
void createSatSprite(TFT_eSprite &spr)
{
  spr.deleteSprite();
  spr.createSprite(16, 20);
  spr.setColorDepth(16);
  spr.fillScreen(TFT_TRANSPARENT);
}

/**
 * @brief Create SNR text sprite
 *
 * @param spr -> Sprite
 */
void createSNRSprite(TFT_eSprite &spr)
{
  spr.deleteSprite();
  spr.createSprite(TFT_WIDTH, 10);
  spr.setColorDepth(8);
  spr.fillScreen(TFT_BLACK);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
}

/**
 * @brief Draw SNR bar and satellite number
 *
 * @param bar -> Bar Control
 * @param barSer -> Bar Control Serie
 * @param id -> Active Sat
 * @param satNum -> Sat ID
 * @param snr -> Sat SNR
 * @param spr -> Sat number sprite
 */
void drawSNRBar(lv_obj_t *bar, lv_chart_series_t *barSer, uint8_t id, uint8_t satNum, uint8_t snr, TFT_eSprite &spr)
{
  lv_point_t p;
  lv_chart_get_y_array(bar, barSer);
  lv_chart_set_value_by_id(bar, barSer, id, snr);
  lv_chart_get_point_pos_by_id(bar, barSer, id, &p);
  spr.setCursor(p.x - 2, 0);
  spr.print(satNum);
}

/**
 * @brief Clear Satellite in View found
 *
 */
void clearSatInView()
{
  for (int clear = 0; clear < MAX_SATELLITES; clear++)
  {
    satTracker[clear].satNum = 0;
    satTracker[clear].elev = 0;
    satTracker[clear].azim = 0;
    satTracker[clear].snr = 0;
    satTracker[clear].active = false;
    satTracker[clear].type = 0;
  }
  createConstelSprite(constelSprite);
  #ifndef TDECK_ESP32S3
    constelSprite.pushSprite(150 * scale, 40 * scale);
  #endif

  #ifdef TDECK_ESP32S3
    constelSprite.pushSprite(250 * scale, 40 * scale);
  #endif
}

/**
 * @brief Display satellite in view info
 *
 * 
 */
void fillSatInView()
{
  for (uint8_t sv = 0; sv < 4; sv++ )
  {
    if (gnssInfoSV[sv].totalMsg.isUpdated())
    {
      // lv_chart_refresh(satelliteBar1);
      // lv_chart_refresh(satelliteBar2);

      for (int i = 0; i < 4; ++i)
      {
        int no = atoi(gnssInfoSV[sv].satNum[i].value());
        if (no >= 1 && no <= MAX_SATELLITES)
        {
          satTracker[no - 1].satNum = atoi(gnssInfoSV[sv].satNum[i].value());
          satTracker[no - 1].elev = atoi(gnssInfoSV[sv].elev[i].value());
          satTracker[no - 1].azim = atoi(gnssInfoSV[sv].azim[i].value());
          satTracker[no - 1].snr = atoi(gnssInfoSV[sv].snr[i].value());
          satTracker[no - 1].active = true;
          satTracker[no - 1].type = sv;
        }
      }

      uint8_t totalMessages = atoi(gnssInfoSV[sv].totalMsg.value());
      uint8_t currentMessage = atoi(gnssInfoSV[sv].msgNum.value());

      if (totalMessages == currentMessage)
      {
        createSNRSprite(spriteSNR1);
        createSNRSprite(spriteSNR2);


        // for (int i = 0; i < (MAX_SATELLLITES_IN_VIEW / 2); i++)
        // {
        //   lv_chart_set_value_by_id(satelliteBar1, satelliteBarSerie1, i, LV_CHART_POINT_NONE);
        //   lv_chart_set_value_by_id(satelliteBar2, satelliteBarSerie2, i, LV_CHART_POINT_NONE);
        // }
      
        uint8_t activeSat = 0;
        for (int i = 0; i < MAX_SATELLITES; ++i)
        {
          if (satTracker[i].active && satTracker[i].snr > 0)
          {
            if (activeSat < (MAX_SATELLLITES_IN_VIEW / 2))
              drawSNRBar(satelliteBar1, satelliteBarSerie1, activeSat, satTracker[i].satNum, satTracker[i].snr, spriteSNR1);
            else
              drawSNRBar(satelliteBar2, satelliteBarSerie2, (activeSat - (MAX_SATELLLITES_IN_VIEW / 2)), satTracker[i].satNum, satTracker[i].snr, spriteSNR2);

            activeSat++;

            satPos = getSatPos(satTracker[i].elev, satTracker[i].azim);

            spriteSat.fillCircle(6, 4, 2, TFT_GREEN);
            spriteSat.setCursor(0 , 8);
            spriteSat.print(i + 1);
            spriteSat.pushSprite(&constelSprite,satPos.x, satPos.y, TFT_TRANSPARENT);

            if ( satTracker[i].posX != satPos.x || satTracker[i].posY != satPos.y)
            {
                spriteSat.fillScreen(TFT_TRANSPARENT);
                spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
            }

            satTracker[i].posX = satPos.x;
            satTracker[i].posY = satPos.y;
          }
          else if ((satTracker[i].active && satTracker[i].snr == 0) || !satTracker[i].active)
          {
            spriteSat.fillScreen(TFT_TRANSPARENT);
            spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
          }
        }
      }
    }
  }

    lv_chart_refresh(satelliteBar1);
    lv_chart_refresh(satelliteBar2);
      
    #ifndef TDECK_ESP32S3
      spriteSNR1.pushSprite(0, 260 * scale);
      spriteSNR2.pushSprite(0, 345 * scale);
    #endif

    #ifdef TDECK_ESP32S3
      spriteSNR1.pushSprite(0, 260);
      spriteSNR2.pushSprite(TFT_WIDTH / 2 , 260);
    #endif 
}
