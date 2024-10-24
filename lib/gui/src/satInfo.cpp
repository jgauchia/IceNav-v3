/**
 * @file satInfo.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Satellites info screen functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "satInfo.hpp"
#include "globalGuiDef.h"

// GSV GPS_GSV; // GPS Satellites in view
// GSV GL_GSV;  // GLONASS Satellites in view
// GSV BD_GSV;  // BEIDOU Satellites in view

SatPos satPos; // Satellite position X,Y in constellation map

TFT_eSprite spriteSat = TFT_eSprite(&tft);        // Sprite for satellite position in map
TFT_eSprite constelSprite = TFT_eSprite(&tft);    // Sprite for Satellite Constellation

lv_obj_t *satelliteBar;               // Satellite Signal Graphics Bars
lv_chart_series_t *satelliteBarSerie; // Satellite Signal Graphics Bars

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
 * @brief Draw SNR bar and satellite number
 *
 * @param bar -> Bar Control
 * @param barSer -> Bar Control Serie
 * @param id -> Active Sat
 * @param satNum -> Sat ID
 * @param snr -> Sat SNR
 */
void drawSNRBar(lv_obj_t *bar, lv_chart_series_t *barSer, uint8_t id, uint8_t satNum, uint8_t snr)
{
  lv_chart_get_y_array(bar, barSer);
  lv_chart_set_value_by_id(bar, barSer, id, snr);
}

/**
 * @brief Clear Satellite in View found
 *
 */
void clearSatInView()
{
  // createConstelSprite(constelSprite);
}

/**
 * @brief Display satellite in view info
 *
 *
 */
void fillSatInView()
{

  lv_chart_refresh(satelliteBar);

    
  for (int i = 0; i < MAX_SATELLLITES_IN_VIEW ; i++)
  {
    lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
  }

  for (int i = 0; i < gpsData.satInView; ++i)
  {
    
    drawSNRBar(satelliteBar, satelliteBarSerie, i, satTracker[i].satNum, satTracker[i].snr);

    satPos = getSatPos(satTracker[i].elev, satTracker[i].azim);

    // spriteSat.fillCircle(6, 4, 2, TFT_GREEN);
    // spriteSat.setCursor(0 , 8);
    // spriteSat.print(satTracker[i].satNum);
    // spriteSat.pushSprite(&constelSprite,satPos.x, satPos.y, TFT_TRANSPARENT);

    // if ( satTracker[i].posX != satPos.x || satTracker[i].posY != satPos.y)
    // {
    //     spriteSat.fillScreen(TFT_TRANSPARENT);
    //     spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
    // }

    satTracker[i].posX = satPos.x;
    satTracker[i].posY = satPos.y;

    // spriteSat.fillScreen(TFT_TRANSPARENT);
    // spriteSat.pushSprite(&constelSprite, satTracker[i].posX, satTracker[i].posY, TFT_TRANSPARENT);
  }

  lv_chart_refresh(satelliteBar);

}
