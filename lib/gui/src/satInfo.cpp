/**
 * @file satInfo.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Satellites info screen functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "satInfo.hpp"
#include "globalGuiDef.h"

SatPos satPos; // Satellite position X,Y in constellation map

TFT_eSprite spriteSat = TFT_eSprite(&tft);        // Sprite for satellite position in map
TFT_eSprite constelSprite = TFT_eSprite(&tft);    // Sprite for Satellite Constellation

lv_obj_t *satelliteBar;               // Satellite Signal Graphics Bars
lv_chart_series_t *satelliteBarSerie; // Satellite Signal Graphics Bars
uint8_t activeSat = 0;                // ID activeSat
uint8_t totalSatView = 0;             // Total Sats in view

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
    satTracker[clear].id = 0;
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
  activeSat = 0;
  for (uint8_t clear = 0; clear < MAX_SATELLITES; clear++)
  {
    satTracker[clear].satNum = 0;
    satTracker[clear].elev = 0;
    satTracker[clear].azim = 0;
    satTracker[clear].snr = 0;
    satTracker[clear].active = false;
    satTracker[clear].type = 0;
    satTracker[clear].id = 0;
  }

  for (uint8_t sv = 0; sv < 3; )
  {
    if (gnssInfoSV[sv].totalMsg.isUpdated())
    {
      uint8_t totalMessages = atoi(gnssInfoSV[sv].totalMsg.value());
      uint8_t currentMessage = atoi(gnssInfoSV[sv].msgNum.value());

      for (uint8_t i = 0; i < 4; ++i)
      {
        uint8_t snr = atoi(gnssInfoSV[sv].snr[i].value());
        if ( snr > 0 )
        {
          satTracker[activeSat].satNum = atoi(gnssInfoSV[sv].satNum[i].value());
          satTracker[activeSat].elev = atoi(gnssInfoSV[sv].elev[i].value());
          satTracker[activeSat].azim = atoi(gnssInfoSV[sv].azim[i].value());
          satTracker[activeSat].snr = snr;
          if ( satTracker[activeSat].snr > 0)
            satTracker[activeSat].active = true;
          else
            satTracker[activeSat].active = false;
          satTracker[activeSat].type = sv;
          satTracker[activeSat].id = activeSat;
          activeSat++;
        }
      }

      if (totalMessages == currentMessage && sv == 2)
      {
        for (int i = 0; i < MAX_SATELLLITES_IN_VIEW; i++)
        {
          lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
        }
        
        totalSatView = activeSat;

        for (int i = 0; i < activeSat; i++)
        {
          satPos = getSatPos(satTracker[i].elev, satTracker[i].azim);
          satTracker[i].posX = satPos.x;
          satTracker[i].posY = satPos.y;

          spriteSat.fillCircle(6, 4, 2, TFT_GREEN);
          spriteSat.setCursor(0 , 8);
          spriteSat.print(i + 1);
          spriteSat.pushSprite(&constelSprite,satPos.x, satPos.y, TFT_TRANSPARENT);

          if ( satTracker[i].posX != satPos.x || satTracker[i].posY != satPos.y)
          {
              spriteSat.fillScreen(TFT_TRANSPARENT);
              spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
          }

          if ( !satTracker[i].active )
          {
            spriteSat.fillScreen(TFT_TRANSPARENT);
            spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
          }

          if ( satTracker[i].active )
            lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, satTracker[i].id, satTracker[i].snr);
        }
      }
    }
    sv++;
  }
}
